// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 4
#define MAX_ITEMS 128
#define LINE_LEN 512

typedef struct {
    char *name;
    char *desc;
    int base_price;
} Item;

Item *items = NULL;
int item_count = 0;

int clients[MAX_CLIENTS];
int current_item = 0;
int highest_bid = 0;
pthread_t *current_bidder = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *receiverThread(void *args);
void *startCountdown(void *args);
void informClient(int clientfd);
void broadcast(const char *msg, int skip_fd);

void broadcast(const char *msg, int skip_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1 && clients[i] != skip_fd) {
            ssize_t w = write(clients[i], msg, strlen(msg));
            (void)w;
        }
    }
}

void free_items() {
    if (!items) return;
    for (int i = 0; i < item_count; ++i) {
        free(items[i].name);
        free(items[i].desc);
    }
    free(items);
    items = NULL;
    item_count = 0;
}

/*
 * args: pointer to a heap-allocated int (clientfd). This thread will free it.
 */
void *startCountdown(void *args) {
    int clientfd = *(int *)args;
    free(args);

    int countdown = 3;
    while (countdown > 0) {
        sleep(5);
        char buf[128];
        snprintf(buf, sizeof(buf), "Current bid %d, going %d\n", highest_bid, countdown);
        broadcast(buf, -1);
        countdown--;
    }

    pthread_mutex_lock(&lock);
    if (current_item < item_count) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Item sold: %s for $%d to Client %d\n",
                 items[current_item].name, highest_bid, clientfd);
        broadcast(msg, -1);

        // notify winner directly
        write(clientfd, "You won the bid!\n", strlen("You won the bid!\n"));
        current_item++;
        // prepare next item if any
        if (current_item < item_count) {
            highest_bid = items[current_item].base_price;
        } else {
            highest_bid = 0;
        }
    }
    pthread_mutex_unlock(&lock);

    // inform all connected clients about the next item if any
    if (current_item < item_count) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1)
                informClient(clients[i]);
        }
    } else {
        broadcast("All items sold. Auction finished.\n", -1);
    }

    return NULL;
}

void *receiverThread(void *args) {
    int clientfd = *(int *)args;
    char buffer[256];

    while (1) {
        int n = read(clientfd, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;
        buffer[n] = '\0';

        // strip trailing newline
        char *nl = strchr(buffer, '\n');
        if (nl) *nl = '\0';

        int bid = atoi(buffer); // non-number -> 0

        pthread_mutex_lock(&lock);
        if (current_item >= item_count) {
            char msg[] = "No active auction currently.\n";
            write(clientfd, msg, strlen(msg));
            pthread_mutex_unlock(&lock);
            continue;
        }

        if (bid <= highest_bid) {
            char msg[128];
            snprintf(msg, sizeof(msg), "Bid too low or equal (current: $%d). Try again\n", highest_bid);
            write(clientfd, msg, strlen(msg));
            pthread_mutex_unlock(&lock);
            continue;
        }

        // accept new highest bid
        highest_bid = bid;
        char info[128];
        snprintf(info, sizeof(info), "Current highest bid: $%d\n", highest_bid);
        broadcast(info, -1);

        // reset/start countdown for this bidder:
        if (current_bidder) {
            // cancel existing countdown
            pthread_cancel(*current_bidder);
            pthread_join(*current_bidder, NULL);
            free(current_bidder);
            current_bidder = NULL;
        }

        // allocate and pass the client's fd by value to the thread
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            pthread_mutex_unlock(&lock);
            continue;
        }
        *arg = clientfd;
        current_bidder = malloc(sizeof(pthread_t));
        if (!current_bidder) {
            free(arg);
            perror("malloc");
            pthread_mutex_unlock(&lock);
            continue;
        }
        if (pthread_create(current_bidder, NULL, startCountdown, arg) != 0) {
            perror("pthread_create");
            free(arg);
            free(current_bidder);
            current_bidder = NULL;
            pthread_mutex_unlock(&lock);
            continue;
        }

        pthread_mutex_unlock(&lock);
    }

    close(clientfd);
    // mark this client slot as -1 in the clients array (main thread holds the array,
    // but since we don't know which slot points to this fd, simply leave it;
    // the main accept loop won't set duplicates because it scans for -1).
    return NULL;
}

void informClient(int clientfd) {
    pthread_mutex_lock(&lock);
    if (current_item >= item_count) {
        write(clientfd, "No items currently up for auction.\n", strlen("No items currently up for auction.\n"));
        pthread_mutex_unlock(&lock);
        return;
    }
    char msg[512];
    snprintf(msg, sizeof(msg),
             "New Item for auction:\nName: %s\nDescription: %s\nBase Price: $%d\n",
             items[current_item].name, items[current_item].desc, items[current_item].base_price);
    write(clientfd, msg, strlen(msg));
    pthread_mutex_unlock(&lock);
}

int load_items_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    items = malloc(sizeof(Item) * MAX_ITEMS);
    if (!items) {
        fclose(f);
        return -1;
    }
    item_count = 0;

    char line[LINE_LEN];
    while (fgets(line, sizeof(line), f) && item_count < MAX_ITEMS) {
        // strip newline
        char *p = line;
        while (*p && (*p == '\r' || *p == '\n')) p++;
        // remove trailing newline
        char *end = line + strlen(line) - 1;
        while (end >= line && (*end == '\n' || *end == '\r')) { *end = '\0'; end--; }

        if (strlen(line) == 0) continue;

        // expected format: name|description|baseprice
        char *name = strtok(line, "|");
        char *desc = strtok(NULL, "|");
        char *price_s = strtok(NULL, "|");

        if (!name || !desc || !price_s) {
            // malformed, skip
            continue;
        }

        items[item_count].name = strdup(name);
        items[item_count].desc = strdup(desc);
        items[item_count].base_price = atoi(price_s);
        item_count++;
    }

    fclose(f);
    return item_count;
}

int main() {
    if (load_items_from_file("auction_items.txt") <= 0) {
        fprintf(stderr, "Failed to load items from auction_items.txt or file empty.\n");
        return 1;
    }

    // set initial highest_bid to the base price of first item
    highest_bid = items[0].base_price;

    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        free_items();
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        free_items();
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        free_items();
        return 1;
    }

    printf("Auction server started on port %d\n", PORT);

    while (1) {
        int clientfd = accept(server_fd, NULL, NULL);
        if (clientfd < 0) {
            perror("accept");
            continue;
        }

        int placed = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1) {
                clients[i] = clientfd;
                pthread_t tid;
                // pass pointer to the clients[i] to thread (slot remains valid)
                pthread_create(&tid, NULL, receiverThread, &clients[i]);
                informClient(clientfd);
                placed = 1;
                break;
            }
        }
        if (!placed) {
            // server full
            char msg[] = "Server full. Try again later.\n";
            write(clientfd, msg, strlen(msg));
            close(clientfd);
        }
    }

    free_items();
    close(server_fd);
    return 0;
}

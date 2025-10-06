// serve.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 4
#define ITEM_COUNT 4

char *items[ITEM_COUNT] = {
    "DaVinci Painting",
    "Mughal Star Carpet",
    "Koh-i-Noor",
    "Queen Elizabeth Crown"
};

int clients[MAX_CLIENTS];
int current_item = 0;
int highest_bid = 0;
pthread_t *current_bidder = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *receiverThread(void *args);
void *startCountdown(void *args);
void informClient(int clientfd);

void broadcast(const char *msg, int skip_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1 && clients[i] != skip_fd) {
            write(clients[i], msg, strlen(msg));
        }
    }
}

void *startCountdown(void *args) {
    int clientfd = *(int *)args;
    int countdown = 5;

    while (countdown > 0) {
        sleep(2);
        char buf[64];
        snprintf(buf, sizeof(buf), "Going... %d\n", countdown);
        broadcast(buf, -1);
        countdown--;
    }

    pthread_mutex_lock(&lock);
    char msg[128];
    snprintf(msg, sizeof(msg),
             "Item sold: %s for $%d to Client %d\n",
             items[current_item], highest_bid, clientfd);
    broadcast(msg, -1);
    write(clientfd, "You won the bid!\n", 18);
    current_item++;
    highest_bid = 0;
    pthread_mutex_unlock(&lock);

    if (current_item < ITEM_COUNT) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1)
                informClient(clients[i]);
        }
    }

    return NULL;
}

void *receiverThread(void *args) {
    int clientfd = *(int *)args;
    char buffer[100];

    while (1) {
        int n = read(clientfd, buffer, sizeof(buffer)-1);
        if (n <= 0) break;
        buffer[n] = '\0';

        int bid = atoi(buffer);

        pthread_mutex_lock(&lock);
        if (bid <= highest_bid) {
            char msg[] = "Bid too low or equal, try again\n";
            write(clientfd, msg, strlen(msg));
            pthread_mutex_unlock(&lock);
            continue;
        }

        highest_bid = bid;
        char info[100];
        snprintf(info, sizeof(info), "Current highest bid: $%d\n", highest_bid);
        broadcast(info, -1);

        if (current_bidder) {
            pthread_cancel(*current_bidder);
            pthread_join(*current_bidder, NULL);
            free(current_bidder);
        }
        current_bidder = malloc(sizeof(pthread_t));
        pthread_create(current_bidder, NULL, startCountdown, &clientfd);
        pthread_mutex_unlock(&lock);
    }

    close(clientfd);
    return NULL;
}

void informClient(int clientfd) {
    char msg[128];
    snprintf(msg, sizeof(msg), "New Item for auction: %s\n", items[current_item]);
    write(clientfd, msg, strlen(msg));
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_CLIENTS);

    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;
    printf("Auction server started on port %d\n", PORT);

    while (1) {
        int clientfd = accept(server_fd, NULL, NULL);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1) {
                clients[i] = clientfd;
                pthread_t tid;
                pthread_create(&tid, NULL, receiverThread, &clients[i]);
                informClient(clientfd);
                break;
            }
        }
    }
}

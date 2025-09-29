/* This code runs the client part of the following question:
 * 
 * Implement an online auction marketplace where the server manages a set 
 * of items listed by an auctioneer. The server is responsible for item listing, 
 * initiating  auctions  with  a  base  price,  tracking  the  highest  bid,  managing 
 * auction  status,  and  finalizing  the  sale.  The  client  participates  in  ongoing 
 * auctions by submitting higher bids, receive real-time updates, and if the bid 
 * is won, then purchase the items. The system should ensure that each item 
 * is first put up for auction, and only sold to the highest bidder after no further 
 * bids are made
 */

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define BUFFER_SIZE 1024
#define ITEM_NAME_LEN 50
#define ITEM_DESC_LEN 200

struct device {
    int fd;
    struct sockaddr_in addr;
    struct device *next;
};

/* Forward */
struct device *initialize_client();
void cleanup_device(struct device *client);

/* Globals used by threads */
static struct device *g_client = NULL;
static volatile sig_atomic_t g_running = 1;

/* Reader thread: prints everything received from server */
void *reader_thread_fn(void *arg) {
    struct device *client = (struct device *)arg;
    char buf[BUFFER_SIZE + 1];

    while (g_running) {
        ssize_t n = recv(client->fd, buf, BUFFER_SIZE, 0);
        if (n > 0) {
            buf[n] = '\0';
            printf("\n[SERVER] %s\n> ", buf);
            fflush(stdout);
        } else if (n == 0) {
            /* server closed connection */
            printf("\n[INFO] Server closed connection.\n");
            g_running = 0;
            break;
        } else {
            if (errno == EINTR) continue;
            perror("reader_thread: recv failed");
            g_running = 0;
            break;
        }
    }
    return NULL;
}

/* Helper: send a text message to server (adds newline optionally) */
int send_text(int fd, const char *msg) {
    if (msg == NULL) return -1;
    size_t len = strlen(msg);
    ssize_t sent = send(fd, msg, len, 0);
    if (sent == -1) return -1;
    return 0;
}

/* Build item string in the format server expects:
   "ITEM_NAME:<name>#ITEM_DESC:<desc>#BASE_PRICE:<price>" */
int send_item(struct device *client) {
    char name[ITEM_NAME_LEN];
    char desc[ITEM_DESC_LEN];
    char price_s[64];
    char payload[BUFFER_SIZE];

    printf("Enter item name: ");
    if (fgets(name, sizeof(name), stdin) == NULL) return -1;
    name[strcspn(name, "\n")] = '\0';

    printf("Enter item description: ");
    if (fgets(desc, sizeof(desc), stdin) == NULL) return -1;
    desc[strcspn(desc, "\n")] = '\0';

    printf("Enter base price (e.g. 20.50): ");
    if (fgets(price_s, sizeof(price_s), stdin) == NULL) return -1;
    price_s[strcspn(price_s, "\n")] = '\0';

    /* format payload */
    int ret = snprintf(payload, sizeof(payload),
                       "ITEM_NAME:%s#ITEM_DESC:%s#BASE_PRICE:%s",
                       name, desc, price_s);
    if (ret < 0 || ret >= (int)sizeof(payload)) {
        fprintf(stderr, "send_item: payload too large\n");
        return -1;
    }

    if (send_text(client->fd, payload) != 0) {
        perror("send_item: send failed");
        return -1;
    }

    printf("[INFO] Item submitted to server.\n");
    return 0;
}

/* Send a numeric bid (as plain text) */
int send_bid(struct device *client) {
    char bid_s[64];
    printf("Enter your bid (e.g. 55.00): ");
    if (fgets(bid_s, sizeof(bid_s), stdin) == NULL) return -1;
    bid_s[strcspn(bid_s, "\n")] = '\0';

    if (strlen(bid_s) == 0) {
        fprintf(stderr, "send_bid: empty bid\n");
        return -1;
    }

    if (send_text(client->fd, bid_s) != 0) {
        perror("send_bid: send failed");
        return -1;
    }

    printf("[INFO] Bid sent: %s\n", bid_s);
    return 0;
}

void print_help(void) {
    puts("Available commands:");
    puts("  list     - List an item (sends ITEM_NAME:..#ITEM_DESC:..#BASE_PRICE:..)");
    puts("  bid      - Send a numeric bid value");
    puts("  quit     - Disconnect and exit");
    puts("  help     - Show this help");
}

/* Catch SIGINT to exit gracefully */
void sigint_handler(int signo) {
    (void)signo;
    g_running = 0;
    if (g_client) {
        shutdown(g_client->fd, SHUT_RDWR);
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    pthread_t reader_thread;

    signal(SIGINT, sigint_handler);

    g_client = initialize_client();
    if (!g_client) {
        fprintf(stderr, "main: could not initialize client\n");
        return EXIT_FAILURE;
    }

    /* start reader thread */
    if (pthread_create(&reader_thread, NULL, reader_thread_fn, g_client) != 0) {
        perror("main: pthread_create failed");
        cleanup_device(g_client);
        return EXIT_FAILURE;
    }

    print_help();
    char cmd[128];

    while (g_running) {
        printf("> ");
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            /* EOF or error (e.g. Ctrl-D) */
            g_running = 0;
            break;
        }
        /* strip newline */
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcmp(cmd, "list") == 0) {
            if (send_item(g_client) != 0) {
                fprintf(stderr, "main: failed to send item\n");
            }
        } else if (strcmp(cmd, "bid") == 0) {
            if (send_bid(g_client) != 0) {
                fprintf(stderr, "main: failed to send bid\n");
            }
        } else if (strcmp(cmd, "quit") == 0) {
            g_running = 0;
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strlen(cmd) == 0) {
            /* ignore empty input */
            continue;
        } else {
            printf("Unknown command: '%s' (type 'help')\n", cmd);
        }
    }

    /* shutdown connection and wait reader thread to finish */
    shutdown(g_client->fd, SHUT_RDWR);
    pthread_join(reader_thread, NULL);

    cleanup_device(g_client);
    printf("Client exiting.\n");
    return EXIT_SUCCESS;
}

/* Implementation of initialize_client and cleanup_device */
struct device *initialize_client() {
    struct device *client = calloc(1, sizeof(struct device));
    if (client == NULL) {
        perror("initialize_client: calloc failed");
        exit(EXIT_FAILURE);
    }

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->fd == -1) {
        perror("initialize_client: socket failed");
        free(client);
        exit(EXIT_FAILURE);
    }

    client->addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDRESS, &client->addr.sin_addr);
    client->addr.sin_port = htons(PORT_NO);
    client->next = NULL;

    if (connect(client->fd, (struct sockaddr *)&client->addr, sizeof(client->addr)) == -1) {
        perror("initialize_client: connect failed");
        close(client->fd);
        free(client);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", IP_ADDRESS, PORT_NO);
    return client;
}

void cleanup_device(struct device *client) {
    if (client == NULL) return;
    if (close(client->fd) == -1) {
        perror("cleanup_device: close failed");
    }
    free(client);
}

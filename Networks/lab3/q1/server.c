/* This code runs a concurrent server on localhost.
 * Responsibilities of a concurrent server:
 *     ➢ The server listens  for client  requests. 
 *     ➢ For each client  request: 
 *          o A new server process is spawned. 
 *     ➢ The  newly  created  process  handles  the
 *        request  independently  and then terminates  upon completion.
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
#include <poll.h>
#include <time.h>
#include <pthread.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024
#define ITEM_COLLECTION_TIME_LIMIT 10       // in seconds
#define ITEM_NAME_LEN 50
#define ITEM_DESC_LEN 200
#define FIRST_BID_TIMEOUT 15                // in seconds
#define BID_TIMEOUT 3                       // in seconds
#define MAX_BID_ATTEMPTS 3                  // Also change abbrevation array accordingly

// Device structure representing a client or server
// @param fd: File descriptor for the socket
// @param addr: Socket address structure
// @param next: Pointer to the next device in the list
struct device {
   int fd;
   struct sockaddr_in addr;
   struct device *next;
};

// Auction state enum
// AUCTION_NOT_STARTED: Auction has not started yet
// WAITING_FOR_ITEMS: Waiting for items to be listed by auctioneer
// AUCTION_STARTED: Auction is currently ongoing
// VOTING: Auction ended, waiting for clients to finalize payments
enum AuctionState {
    AUCTION_NOT_STARTED,
    WAITING_FOR_ITEMS,
    AUCTION_STARTED,
    VOTING
};

// Item status enum
// ITEM_NOT_STARTED: Item is not yet put up for auction
// ITEM_AUCTIONNING: Item is currently being auctioned
// ITEM_SOLD: Item is sold to the highest bidder
// ITEM_UNSOLD: Item is not sold (no bids received)
enum ItemStatus {
    ITEM_NOT_STARTED,
    ITEM_AUCTIONNING,
    ITEM_SOLD,
    ITEM_UNSOLD
};

// Auction item structure
// @param item_id: Unique identifier for the item
// @param item_name: Name of the item
// @param item_desc: Description of the item
// @param base_price: Minimum price to start the auction
// @param status: Current status of the item enum: (not started, auctionning, sold, unsold)
// @param highest_bid: Current highest bid for the item
// @param highest_bidder: Pointer to the device structure of the highest bidder
// @param next: Pointer to the next item in the list
struct auction_item {
    // auction item details
    int item_id;
    char item_name[ITEM_NAME_LEN];
    char item_desc[ITEM_DESC_LEN];
    float base_price;

    // auction item selling details
    enum ItemStatus status;
    float highest_bid;
    struct device *highest_bidder;

    // pointer to next item in the list
    struct auction_item *next;
};

// Client and server initialization, cleanup and related functions
struct device *initialize_server();
struct device *accept_client(struct device *);
struct device *get_prev_client_in_list(struct device *);
struct device *get_next_client_in_list(struct device *);
struct device *get_device_from_fd(int);
struct pollfd *construct_pollfd_array_from_device_list(nfds_t *);
void message_client(struct device *, char *);
void broadcast_message(char *);
void close_all_clients();
void cleanup_client(struct device *);
void cleanup_server(struct device *);

// Auction related functions
void start_auction();
void get_auction_items_from_clients();

// Auction item related functions
struct auction_item *parse_auction_item_from_buffer(char *);
void add_auction_item_to_list(struct auction_item *);
void conduct_auction_for_item(struct auction_item *);
void auction_item_cleanup();

// Global variables
pthread_rwlock_t device_list_lock;
struct device *device_list_head = NULL;

pthread_rwlock_t auction_item_list_lock;
int auction_item_count = 0;
struct auction_item *auction_item_list_head = NULL;
struct auction_item *current_auction_item = NULL;  

enum AuctionState auctionstate = AUCTION_NOT_STARTED;

static const char *abbrevation[] = {'once', 'twice', 'and thrice'};
 
int main (int argc, char **argv) {


    return 0;
}

struct device *initialize_server() {
    struct device *server = calloc(1, sizeof(struct device));
    if (server == NULL) {
        perror("initialize_server: calloc failed");
        exit(EXIT_FAILURE);
    }

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton(AF_INET, IP_ADDRESS, &server->addr.sin_addr);
    server->addr.sin_port = htons(PORT_NO);

    if (bind(server->fd, (struct sockaddr *) &server->addr, sizeof(server->addr)) == -1) {
        perror("initialize_server: bind failed");
        close(server->fd);
        free(server);
        exit(EXIT_FAILURE);
    }

    if (listen(server->fd, LISTEN_BACKLOG) == -1) {
        perror("initialize_server: listen failed");
        close(server->fd);
        free(server);
        exit(EXIT_FAILURE);
    }

    // initialize rwlocks
    if (pthread_rwlock_init(&device_list_lock, NULL) != 0) {
        perror("initialize_server: pthread_rwlock_init failed");
        close(server->fd);
        free(server);
        exit(EXIT_FAILURE);
    }
    if (pthread_rwlock_init(&auction_item_list_lock, NULL) != 0) {
        perror("initialize_server: pthread_rwlock_init failed");
        pthread_rwlock_destroy(&device_list_lock);
        close(server->fd);
        free(server);
        exit(EXIT_FAILURE);
    }

    return server;
}

struct device *accept_client(struct device *server) {
    struct device *client = calloc(1, sizeof(struct device));
    if (client == NULL) {
        perror("accept_client: calloc failed");
        return NULL;
    }

    socklen_t client_sockaddr_in_size = sizeof(client->addr);
    client->fd = accept(server->fd, (struct sockaddr*) &client->addr, &client_sockaddr_in_size);
    if (client->fd == -1) {
        perror("accept_client: accept failed");
        free(client);
        return NULL;
    }

    client->next = NULL;

    // add client to the end of the device list
    pthread_rwlock_wrlock(&device_list_lock);
    struct device *itr = device_list_head;
    if (itr == NULL) {
        device_list_head = client;
    } else {
        while (itr->next != NULL) {
            itr = itr->next;
        }
        itr->next = client;
    }
    pthread_rwlock_unlock(&device_list_lock);

    return client;
}

// NOTE: Caller must lock device_list_lock before calling this function
struct device *get_prev_client_in_list(struct device *client) {
    struct device *itr = device_list_head;

    // if client is the head of the list
    if (itr == NULL || itr == client) {
        return NULL;
    }

    while (itr->next != NULL && itr->next != client) {
        itr = itr->next;
    }

    // if client is not present
    if (itr->next == NULL) {
        return NULL;
    }

    // here itr->next == client;
    return itr;
}

// NOTE: Caller must lock device_list_lock before calling this function
struct device *get_next_client_in_list(struct device *client) {
    if (client != NULL) {
        return client->next;
    }
    return NULL;
}

struct pollfd *construct_pollfd_array_from_device_list(nfds_t *count) {
    // count number of clients in the list
    *count = 0;

    pthread_rwlock_rdlock(&device_list_lock);

    struct device *itr = device_list_head;
    while (itr != NULL) {
        (*count)++;
        itr = itr->next;
    }

    struct pollfd *pfds = calloc(*count, sizeof(struct pollfd));
    if (pfds == NULL) {
        perror("construct_pollfd_array_from_device_list: calloc failed");
        return NULL;
    }

    itr = device_list_head;
    for (nfds_t i = 0; i < *count; i++) {
        pfds[i].fd = itr->fd;
        pfds[i].events = POLLIN;          // check for data to read
        itr = itr->next;
    }

    pthread_rwlock_unlock(&device_list_lock);

    return pfds;
}

struct device *get_device_from_fd(int fd) {
    pthread_rwlock_rdlock(&device_list_lock);
    struct device *itr = device_list_head;
    while (itr != NULL) {
        if (itr->fd == fd) {
            pthread_rwlock_unlock(&device_list_lock);
            return itr;
        }
        itr = itr->next;
    }
    pthread_rwlock_unlock(&device_list_lock);
    return NULL;
}

void message_client(struct device *client, char *message) {
    if (client == NULL || message == NULL) {
        fprintf(stderr, "message_client: client or message is NULL\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%s\n", message);
    ssize_t bytes_sent = send(client->fd, buffer, BUFFER_SIZE, 0);
    if (bytes_sent == -1) {
        perror("message_client: send failed");
    }
}

void broadcast_message(char *message) {
    if (message == NULL) {
        fprintf(stderr, "broadcast_message: message is NULL\n");
        return;
    }

    pthread_rwlock_rdlock(&device_list_lock);
    struct device *itr = device_list_head;
    while (itr != NULL) {
        message_client(itr, message);
        itr = itr->next;
    }
    pthread_rwlock_unlock(&device_list_lock);
}

void close_all_clients() {
    pthread_rwlock_wrlock(&device_list_lock);
    struct device *itr = device_list_head;
    struct device *next;
    while (itr != NULL) {
        next = itr->next;
        if (close(itr->fd) == -1) {
            perror("close_all_clients: unable to close file descriptor");
        }
        free(itr);
        itr = next;
    }
    device_list_head = NULL;
    pthread_rwlock_unlock(&device_list_lock);
}

void cleanup_client(struct device *client) {
    // remove the client from the list `device_list_head`
    pthread_rwlock_wrlock(&device_list_lock);
    struct device *prev = get_prev_client_in_list(client);
    prev->next = get_next_client_in_list(client);
    pthread_rwlock_unlock(&device_list_lock);

    if (close(client->fd) == -1) {
        perror("cleanup_client: unable to close file descriptor");
    }
    free(client);
}

void cleanup_server(struct device *server) {
    close_all_clients();
    auction_item_cleanup(auction_item_list_head);

    if (close(server->fd) == -1) {
        perror("cleanup_server: unable to close file descriptor");
    }
    free(server);

    pthread_rwlock_destroy(&device_list_lock);
    pthread_rwlock_destroy(&auction_item_list_lock);
}

// Auction related functions

// This fucntion will implement all the stages in an auction
// Stage 1: Waiting for Items
// Stage 2: Auctionning Items
// Stage 3: Finalizing payments
// Go to Stage 2 while items left
void start_auction() {
    // Wait for Items
    auctionstate = WAITING_FOR_ITEMS;
    get_auction_items_from_clients();

    // Auctionning Items
    pthread_rwlock_rdlock(&auction_item_list_lock);
    auctionstate = AUCTION_STARTED;
    struct auction_item *itr = auction_item_list_head;
    while (itr != NULL) {
        // auction the item `itr`
        conduct_auction_for_item(itr);
        itr = itr->next;
    }
    pthread_rwlock_unlock(&auction_item_list_lock);
}

// This function will get auction items from clients
// All items need to be collected before starting the auction
// And collection should be done within ITEM_COLLECTION_TIME_LIMIT
void get_auction_items_from_clients() {
    time_t start_time = time(NULL);
    while (difftime(time(NULL), start_time) < ITEM_COLLECTION_TIME_LIMIT) {
        nfds_t pollFdsCount = 0;
        struct pollfd *pfds = construct_pollfd_array_from_device_list(&pollFdsCount);
        if (pfds == NULL) {
            continue;
        }

        int wait_time = (int) ((ITEM_COLLECTION_TIME_LIMIT - difftime(time(NULL), start_time)) * 1000);   // in milliseconds
        if (wait_time < 0) {
            break;
        }

        int ret = poll(pfds, pollFdsCount, wait_time);

        if (ret < 0) {
            perror("get_auction_items_from_clients: poll failed");
            free(pfds);
            continue;
        } else if (ret == 0) {
            // Inform all clients that item collection time is over
            broadcast_message("Item collection time is over");
            broadcast_message("Auction is starting now");
            free(pfds);
            break;
        } else {
            // some fds are ready
            for (nfds_t i = 0; i < pollFdsCount; i++) {
                if (pfds[i].revents & POLLIN) {
                    // data to read on pfds[i].fd
                    char buffer[BUFFER_SIZE];
                    memset(buffer, '\0', BUFFER_SIZE);
                    ssize_t bytes_received = recv(pfds[i].fd, buffer, BUFFER_SIZE - 1, 0);
                    if (bytes_received < 0) {
                        perror("get_auction_items_from_clients: recv failed");
                        continue;
                    } else if (bytes_received == 0) {
                        // connection closed by client
                        printf("Client disconnected, cleaning up\n");
                        struct device *client = get_device_from_fd(pfds[i].fd);
                        if (client != NULL) {
                            cleanup_client(client);
                        }
                        continue;
                    } else {
                        buffer[bytes_received] = '\0';
                        struct auction_item *item = parse_auction_item_from_buffer(buffer);
                        if (item != NULL) {
                            add_auction_item_to_list(item);
                            // tell client that item is added successfully
                            message_client(get_device_from_fd(pfds[i].fd), "Item added successfully");
                        } else {
                            fprintf(stderr, "get_auction_items_from_clients: parse_auction_item_from_buffer failed\n");
                            // tell client that item addition failed
                            message_client(get_device_from_fd(pfds[i].fd), "Item addition failed");
                        }
                    }
                }
            }
        }
        free(pfds);
    }
}


// Auction item related functions

// Takes char buffer, read from client, as input and
// creates the auction_item struct with the details
// buffer format: "ITEM_NAME:<name>#ITEM_DESC:<desc>#BASE_PRICE:<price>"
struct auction_item *parse_auction_item_from_buffer(char *buffer) {

    struct auction_item *item = calloc(1, sizeof(struct auction_item));
    if (item == NULL) {
        perror("parse_auction_item_from_buffer: calloc failed");
        return;
    }

    char *token = strtok(buffer, "#");
    while (token != NULL) {
        if (strncmp(token, "ITEM_NAME:", 10) == 0) {
            strncpy(item->item_name, token + 10, ITEM_NAME_LEN - 1);
            item->item_name[ITEM_NAME_LEN - 1] = '\0'; // Ensure null-termination
        } else if (strncmp(token, "ITEM_DESC:", 10) == 0) {
            strncpy(item->item_desc, token + 10, ITEM_DESC_LEN - 1);
            item->item_desc[ITEM_DESC_LEN - 1] = '\0'; // Ensure null-termination
        } else if (strncmp(token, "BASE_PRICE:", 11) == 0) {
            item->base_price = atof(token + 11);
        }
        token = strtok(NULL, "#");
    }

    item->item_id = auction_item_count++;
    item->status = ITEM_NOT_STARTED;
    item->highest_bid = 0.0;
    item->highest_bidder = NULL;
    item->next = NULL;

    return item;
}

void add_auction_item_to_list(struct auction_item *item) {
    pthread_rwlock_wrlock(&auction_item_list_lock);
    struct auction_item *itr = auction_item_list_head;
    if (itr == NULL) {
        auction_item_list_head = item;
    } else {
        while (itr->next != NULL) {
            itr = itr->next;
        }
        itr->next = item;
    }
    pthread_rwlock_unlock(&auction_item_list_lock);
}

// This function will conduct auction for a single item
// server will wait for a bid to be placed
// for first bid, it will wait for FIRST_BID_TIMEOUT seconds
// if a bid is placed:
//      it will wait for BID_TIMEOUT seconds for MAX_BID_ATTEMPTS times
//          each time broadcasting highest bid so far and number of attempts left in MAX_BID_ATTEMPTS
//      if no bid is placed in these MAX_BID_ATTEMPTS attempts, auction ends and item is sold to highest bidder (if any)
//      if a bid is placed in any of these MAX_BID_ATTEMPTS attempts, repeat the process
// if no bid is placed in FIRST_BID_TIMEOUT seconds, auction ends and item is unsold
void conduct_auction_for_item(struct auction_item *item) {
    if (item == NULL) {
        fprintf(stderr, "conduct_auction_for_item: item is NULL\n");
        return;
    }

    item->status = ITEM_AUCTIONNING;
    current_auction_item = item;

    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Auction started for item: %s with base price: %.2f", item->item_name, item->base_price);
    broadcast_message(message);
    snprintf(message, BUFFER_SIZE, "Item description: %s", item->item_desc);
    broadcast_message(message);
    snprintf(message, BUFFER_SIZE, "Please place your bids!");
    broadcast_message(message);

    time_t auction_start_time = time(NULL);
    time_t last_bid_time = auction_start_time;
    int bid_attempts_left = MAX_BID_ATTEMPTS;
    int waiting_for_first_bid = 1;

    while (bid_attempts_left > 0) {
        nfds_t pollFdsCount = 0;
        struct pollfd *pfds = construct_pollfd_array_from_device_list(&pollFdsCount);
        if (pfds == NULL) {
            continue;
        }

        int wait_time;
        if (waiting_for_first_bid) {
            wait_time = (int) (FIRST_BID_TIMEOUT * 1000);   // in milliseconds
        } else if (bid_attempts_left == 1) {
            wait_time = 0;
        } else {
            wait_time = (int) (BID_TIMEOUT * 1000);   // in milliseconds
        }

        int ret = poll(pfds, pollFdsCount, wait_time);

        if (ret < 0) {
            perror("conduct_auction_for_item: poll failed");
            free(pfds);
            continue;
        } else if (ret == 0) {
            // timeout
            free(pfds);
            if (waiting_for_first_bid) {
                // no bid placed in FIRST_BID_TIMEOUT seconds
                snprintf(message, BUFFER_SIZE, "No bids received for item: %s. Item is unsold.", item->item_name);
                broadcast_message(message);
                item->status = ITEM_UNSOLD;
                return;
            } else {
                // no bid placed in BID_TIMEOUT seconds
                bid_attempts_left--;
                if (bid_attempts_left == 0) {
                    // auction ends
                    if (item->highest_bidder != NULL) {
                        snprintf(message, BUFFER_SIZE, "Auction ended for item: %s. Sold to highest bidder with bid: %.2f", item->item_name, item->highest_bid);
                        broadcast_message(message);
                        item->status = ITEM_SOLD;
                    } else {
                        snprintf(message, BUFFER_SIZE, "Auction ended for item: %s. No bids received. Item is unsold.", item->item_name);
                        broadcast_message(message);
                        item->status = ITEM_UNSOLD;
                    }
                    return;
                } else {
                    // snprintf(message, BUFFER_SIZE, "No new bids received. %d bid attempts left for item: %s. Current highest bid: %.2f", bid_attempts_left, item->item_name, item->highest_bid);
                    snprintf(message, BUFFER_SIZE, "Item: %s | Current highest bid: %.2f | Locking in %s...", item->item_name, item->highest_bid, abbrevation[MAX_BID_ATTEMPTS - bid_attempts_left - 1]);
                    broadcast_message(message);
                }
            }
        } else {
            // some fds are ready
            int bid_received = 0;
            for (nfds_t i = 0; i < pollFdsCount; i++) {
                if (pfds[i].revents & POLLIN) {
                    // data to read on pfds[i].fd
                    char buffer[BUFFER_SIZE];
                    memset(buffer, '\0', BUFFER_SIZE);
                    ssize_t bytes_received = recv(pfds[i].fd, buffer, BUFFER_SIZE - 1, 0);
                    if (bytes_received < 0) {
                        perror("conduct_auction_for_item: recv failed");
                        continue;
                    } else if (bytes_received == 0) {
                        // connection closed by client
                        printf("Client disconnected, cleaning up\n");
                        struct device *client = get_device_from_fd(pfds[i].fd);
                        if (client != NULL) {
                            cleanup_client(client);
                        }
                        continue;
                    } else {
                        buffer[bytes_received] = '\0';
                        float bid = atof(buffer);
                        if (bid > item->highest_bid && bid >= item->base_price) {
                            item->highest_bid = bid;
                            item->highest_bidder = get_device_from_fd(pfds[i].fd);
                            last_bid_time = time(NULL);
                            snprintf(message, buffer, "New highest bid: %.2f for item: %s", item->highest_bid, item->item_name);
                            broadcast_message(message);
                            bid_received = 1;
                        } else {
                            message_client(get_device_from_fd(pfds[i].fd), "Bid too low");
                        }
                    }
                }
            }
            free(pfds);
            if (bid_received) {
                // reset bid attempts
                bid_attempts_left = MAX_BID_ATTEMPTS;
                waiting_for_first_bid = 0;
            }
        }
        free(pfds);
    }
}

void auction_item_cleanup() {
    pthread_rwlock_wrlock(&auction_item_list_lock);
    struct auction_item *itr = auction_item_list_head;
    struct auction_item *next;
    while (itr != NULL) {
        next = itr->next;
        free(itr);
        itr = next;
    }
    auction_item_list_head = NULL;
    pthread_rwlock_unlock(&auction_item_list_lock);
}
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024
#define ITEM_COLLECTION_TIME_LIMIT 10          // in seconds

struct device {
   int fd;
   struct sockaddr_in addr;
   struct device *next;
};

enum AuctionState {
    AUCTION_NOT_STARTED,
    WAITING_FOR_ITEMS,
    AUCTION_STARTED,
    VOTING
};

enum ItemStatus {
    ITEM_NOT_STARTED,
    ITEM_AUCTIONNING,
    ITEM_SOLD,
    ITEM_UNSOLD
};

struct auction_item {
    int item_id;
    char item_name[50];
    char item_desc[200];
    float base_price;
    float highest_bid;
    struct device *highest_bidder;
    enum ItemStatus status;
    struct auction_item *next;
};

// Client and server initialization, cleanup and related functions
struct device *initialize_server();
struct device *accept_client(struct device *);
struct device *get_prev_client_in_list(struct device *);
struct device *get_next_client_in_list(struct device *);
struct pollfd *construct_pollfd_array_from_device_list(nfds_t *);
void cleanup_device(struct device *);

// Auction related functions
void start_auction();
void get_auction_items_from_clients();

// Global variables
struct device *device_list_head = NULL;
struct auction_item *auction_item_list_head = NULL;
struct auction_item *current_auction_item = NULL;               // is this needed?
enum AuctionState auctionstate = AUCTION_NOT_STARTED;
 
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

    struct device *itr = device_list_head;
    if (itr == NULL) {
        device_list_head = client;
    } else {
        while (itr->next != NULL) {
            itr = itr->next;
        }
        itr->next = client;
    }

    return client;
}

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

struct device *get_next_client_in_list(struct device *client) {
    if (client != NULL) {
        return client->next;
    }
    return NULL;
}

struct pollfd *construct_pollfd_array_from_device_list(nfds_t *count) {
    // count number of clients in the list
    *count = 0;
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

    return pfds;
}

void cleanup_device(struct device *client) {
    // remove the client from the list `device_list_head`
    struct device *prev = get_prev_client_in_list;
    prev->next = get_next_client_in_list;

    if (close(client->fd) == -1) {
        perror("cleanup_device: unable to close file descriptor");
    }
    free(client);
}

// Auction related functions

// This fucntion will implement all the stages in an auction
// Stage 1: Waiting for Items
// Stage 2: Auctionning Items
// Stage 3: Finalizing payments
// Go to Stage 2 while items left
void start_auction() {
    // Wait for Items
    

    // start auction of each item
}

// This function will get auction items from clients
// All items need to be collected before starting the auction
// And collection should be done within ITEM_COLLECTION_TIME_LIMIT
void get_auction_items_from_clients() {
    time_t start_time = time(NULL);
    while (difftime(time(NULL), start_time) < ITEM_COLLECTION_TIME_LIMIT) {
        
    }
}
// network_utils.c

#include "../include/network_utils.h"
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

// Global Variables
pthread_rwlock_t device_list_lock;
struct device *device_list_head = NULL;
struct device *tcp_server = NULL;
struct device *tcp_client = NULL;

// Initializers
extern struct device *initialize_tcp_server(const char *ip_address, int port) {    
    struct device *server = calloc(1, sizeof(struct device));
    if (server == NULL) {
        perror("initialize_tcp_server: calloc failed");
        return NULL;
    }

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->fd == -1) {
        perror("initialize_tcp_server: Socket Creation Failed!");
        free(server);
        return NULL;
    }

    // Allow immediate reuse of the address after the server terminates
    int opt = 1;
    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("initialize_tcp_server: setsockopt failed");
        free(server);
        return NULL;
    }

    server->addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_address, &server->addr.sin_addr) <= 0) {
        perror("initialize_tcp_server: inet_pton failed");
        close(server->fd);
        free(server);
        return NULL;
    }
    server->addr.sin_port = htons(port);

    if (bind(server->fd, (struct sockaddr *) &server->addr, sizeof(server->addr)) == -1) {
        perror("initialize_tcp_server: bind failed");
        close(server->fd);
        free(server);
        return NULL;
    }

    if (listen(server->fd, LISTEN_BACKLOG) == -1) {
        perror("initialize_tcp_server: listen failed");
        close(server->fd);
        free(server);
        return NULL;
    }

    server->next = NULL;

    // initialize signal handlers
    initialize_signal_handler();

    // initialize rwlocks
    if (pthread_rwlock_init(&device_list_lock, NULL) != 0) {
        perror("initialize_tcp_server: pthread_rwlock_init failed");
        close(server->fd);
        free(server);
        return NULL;
    }

    return server;
}

extern struct device *initialize_tcp_client(const char *ip_address, int port) {
    struct device *client = calloc(1, sizeof(struct device));
    if (client == NULL) {
        perror("initialize_tcp_client: calloc failed");
        return NULL;
    }

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->fd == -1) {
        perror("initialize_tcp_client: Socket Creation Failed!");
        free(client);
        return NULL;
    }

    client->addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_address, &client->addr.sin_addr) <= 0) {
        perror("initialize_tcp_client: inet_pton failed");
        close(client->fd);
        free(client);
        return NULL;
    }
    client->addr.sin_port = htons(port);

    if (connect(client->fd, (struct sockaddr *) &client->addr, sizeof(client->addr)) == -1) {
        perror("initialize_tcp_client: connect failed");
        close(client->fd);
        free(client);
        return NULL;
    }

    client->next = NULL;

    // initialize signal handlers
    initialize_signal_handler();

    return client;
}

extern struct device *accept_client(struct device *server) {
    if (server == NULL) {
        fprintf(stderr, "accept_client: server is NULL\n");
        return NULL;
    }

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

    // add client to the device list
    pthread_rwlock_wrlock(&device_list_lock);
    if (device_list_head == NULL) {
        device_list_head = client;
    } else {
        struct device *itr = device_list_head;
        while (itr->next != NULL) {
            itr = itr->next;
        }
        itr->next = client;
    }
    pthread_rwlock_unlock(&device_list_lock);

    return client;
}

extern void initialize_signal_handler() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
}

// Sending Messages
extern void message_client(struct device *client, const char *message_bufffer) {
    if (client == NULL || message_bufffer == NULL) {
        fprintf(stderr, "message_client: client or message_buffer is NULL\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    int n = snprintf(buffer, BUFFER_SIZE, "%s\n", message_bufffer);
    if (n < 0 || n >= BUFFER_SIZE) {
        fprintf(stderr, "message_client: message too long\n");
        return;
    }

    ssize_t bytes_sent = send(client->fd, buffer, (size_t)n, 0);
    if (bytes_sent == -1) {
        perror("message_client: send failed");
    }
}

extern void message_client_fd(int client_fd, const char *message_buffer) {
    if (client_fd < 0 || message_buffer == NULL) {
        fprintf(stderr, "message_client_fd: invalid client_fd or message_buffer is NULL\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    int n = snprintf(buffer, BUFFER_SIZE, "%s\n", message_buffer);
    if (n < 0 || n >= BUFFER_SIZE) {
        fprintf(stderr, "message_client_fd: message too long\n");
        return;
    }

    ssize_t bytes_sent = send(client_fd, buffer, (size_t)n, 0);
    if (bytes_sent == -1) {
        perror("message_client_fd: send failed");
    }
}
extern void broadcast_message(const char *message) {
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

// Device Management
extern struct device *get_device_from_fd(int fd) {
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
extern struct device *get_prev_client_in_list(struct device *client) {
    if (client == NULL) {
        fprintf(stderr, "get_prev_client_in_list: client is NULL\n");
        return NULL;
    }

    struct device *itr = device_list_head;
    struct device *prev = NULL;
    while (itr != NULL) {
        if (itr == client) {
            return prev;
        }
        prev = itr;
        itr = itr->next;
    }
    return NULL; // client not found in list
}

extern struct device *get_next_client_in_list(struct device *client) {
    if (client == NULL) {
        fprintf(stderr, "get_next_client_in_list: client is NULL\n");
        return NULL;
    }
    return client->next;
}

// Cleanup
extern void cleanup_client(struct device *client) {
    if (client == NULL) {
        fprintf(stderr, "cleanup_client: client is NULL\n");
        return;
    }

    pthread_rwlock_wrlock(&device_list_lock);
    struct device *prev = get_prev_client_in_list(client);
    if (prev == NULL) {
        // client is head
        device_list_head = client->next;
    } else {
        prev->next = client->next;
    }
    pthread_rwlock_unlock(&device_list_lock);

    shutdown(client->fd, SHUT_RDWR);
    if (close(client->fd) == -1) {
        perror("cleanup_client: close failed");
    }
    free(client);
}

extern void cleanup_server(struct device *server) {
    if (server == NULL) {
        fprintf(stderr, "cleanup_server: server is NULL\n");
        return;
    }

    // close all clients
    cleanup_all_clients();

    if (close(server->fd) == -1) {
        perror("cleanup_server: close failed");
    }
    free(server);

    // destroy rwlock
    if (pthread_rwlock_destroy(&device_list_lock) != 0) {
        perror("cleanup_server: pthread_rwlock_destroy failed");
    }
}

extern void cleanup_all_clients() {
    pthread_rwlock_wrlock(&device_list_lock);
    struct device *itr = device_list_head;
    struct device *next;
    while (itr != NULL) {
        next = itr->next;
        if (close(itr->fd) == -1) {
            perror("cleanup_all_clients: close failed");
        }
        free(itr);
        itr = next;
    }
    device_list_head = NULL;
    pthread_rwlock_unlock(&device_list_lock);
}

extern void handle_signal(int sig) {
    printf("Caught signal %d, cleaning up and exiting...\n", sig);
    
    if (tcp_server != NULL) {
        cleanup_server(tcp_server);
    }
    exit(EXIT_SUCCESS);
}

// Other extra Utitility Functions
int max(int a, int b) {
    return (a > b) ? a : b;
}
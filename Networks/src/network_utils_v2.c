// network_utils.c

#include "../include/network_utils_v2.h"
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
#include <stdarg.h>

// Global Variables
pthread_rwlock_t device_list_lock;
struct device *device_list_head = NULL;
struct device *tcp_server = NULL;
struct device *tcp_client = NULL;
int debug = 0;

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

// Concurrent client management
extern int concurrently_handle_clients_with_handler(void *(*__start_routine)(void *), int log_level) {
    // check if tcp_server has been initialized
    if (tcp_server == NULL) {
        return -1;
    }

    while (1) {
        struct device *new_client = accept_client(tcp_server);

        if (new_client == NULL) continue;

        // Print connected client details on console
        if (log_level > 0) {
            char *client_ip_address = get_client_ip(new_client);
            if (client_ip_address == NULL) {
                if (log_level > 1) {
                    perror("concurrently_handle_clients_with_handler: get_client_ip failed");
                }
                continue;
            }
            int client_port = get_client_port(new_client);
            if (client_port == 0) {
                if (log_level > 1) {
                    perror("concurrently_handle_clients_with_handler: get_client_port failed");
                }
                continue;
            }
            printf("Connected to clinet IP: %s Port: %d", client_ip_address, ntohs(new_client->addr.sin_port));
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, __start_routine, new_client) != 0) {
            if (log_level > 1) {
                perror("concurrently_handle_clients_with_handler: pthread_create error");
            }
            continue;
        }

        // if tcp server stoped, break
        if (tcp_server == NULL) {
            break;
        }
    }

    return 0;
}

// Sending Messages
extern int message_device(struct device *client, uint16_t type, const char *payload, size_t payload_size) {
    if (client == NULL) {
        errno = EINVAL;
        return -EINVAL;
    }

    struct MessageHeader header;
    header.type = htons(type);
    header.length = payload_size;

    // send header
    int ret_val = send(client->fd, &header, sizeof(header), 0);
    if (ret_val < 0) {
        return ret_val;
    }

    // send payload if any
	if (debug > 2) {
    	fprintf(stderr, "payload = %s^^^%ld^^\n", payload, payload_size);
	}
    if (payload && payload_size > 0) {
        char buffer[BUFFER_SIZE];
        strncpy(buffer, payload, BUFFER_SIZE);
        ret_val = send(client->fd, buffer, BUFFER_SIZE, 0);
        if (ret_val < 0) {
            return ret_val;
        }
    }

    return ret_val;
}

extern void depriciated_message_device_fd(int client_fd, const char *message_buffer) {
    if (client_fd < 0 || message_buffer == NULL) {
        fprintf(stderr, "message_device_fd: invalid client_fd or message_buffer is NULL\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    int n = snprintf(buffer, BUFFER_SIZE, "%s\n", message_buffer);
    if (n < 0 || n >= BUFFER_SIZE) {
        fprintf(stderr, "message_device_fd: message too long\n");
        return;
    }

    ssize_t bytes_sent = send(client_fd, buffer, (size_t)n, 0);
    if (bytes_sent == -1) {
        perror("message_device_fd: send failed");
    }
}
extern void broadcast_message(const char *message, uint16_t type) {
    if (message == NULL) {
        fprintf(stderr, "broadcast_message: message is NULL\n");
        return;
    }

    pthread_rwlock_rdlock(&device_list_lock);
    struct device *itr = device_list_head;
    while (itr != NULL) {
        message_device_formatted(itr, type, message);
        itr = itr->next;
    }
    pthread_rwlock_unlock(&device_list_lock);
}

extern int message_device_formatted(struct device *client, uint16_t type, const char *fmt, ...) {
    char buffer[BUFFER_SIZE];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (n < 0) {
        errno = EILSEQ;     // Illegal byte sequence (common choice for encoding errors)
        return -EILSEQ;
    }
    if (n >= BUFFER_SIZE) {
        errno = EMSGSIZE;   // Message too long
        return -EMSGSIZE;
    }

    return message_device(client, type, buffer, n);
}

// Get message from client
extern ssize_t receive_message(struct device *client, char *payload, size_t size, uint16_t *type) {
    if (client == NULL || payload == NULL || size <= 0) {
        errno = EINVAL;
        return -1;
    }

    struct MessageHeader header;
    ssize_t header_bytes = recv(client->fd, &header, sizeof(header), 0);
    if (header_bytes <= 0) {
        return header_bytes;
    }

    *type = ntohs(header.type);
    uint16_t lenght = ntohs(header.length);

    if (lenght >= size) {
        lenght = size - 1;
    }

    memset(payload, '\0', size);
    if (lenght > 0) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client->fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            return bytes_received;
        }
        
        strncpy(payload, buffer, lenght);
        payload[bytes_received] = '\0';
		if (debug > 2) {
        	fprintf(stderr, "%s", payload);
		}
        return bytes_received;
    } else {
        return lenght;
    }
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

extern char *get_client_ip(struct device *client) {
    if (client == NULL) {
        errno = EINVAL;
        return NULL;
    }

    // char *ip_str = calloc(INET_ADDRSTRLEN + 1, sizeof(char));
    // if (ip_str == NULL) {
    //     return NULL;    // errno set by calloc
    // }
    static _Thread_local char ip_str[INET_ADDRSTRLEN + 1];

    if (inet_ntop(client->addr.sin_family, &(client->addr.sin_addr), ip_str, INET_ADDRSTRLEN) == NULL) {
        return NULL;    // errno set by inet_ntop
    }

    return ip_str;
}

extern uint16_t get_client_port(struct device *client) {
    if (client == NULL) {
        errno = EINVAL;
        return 0;
    }

    return ntohs(client->addr.sin_port);
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
    printf("\nCaught signal %d, cleaning up and exiting...\n", sig);
    
    if (tcp_server != NULL) {
        cleanup_server(tcp_server);
    }
    if (tcp_client != NULL) {
        cleanup_client(tcp_client);
    }
    exit(EXIT_SUCCESS);
}

// Other extra Utitility Functions
int max(int a, int b) {
    return (a > b) ? a : b;
}

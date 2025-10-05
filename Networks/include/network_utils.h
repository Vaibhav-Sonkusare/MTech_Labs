// network_utils.h

#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H 

#define _XOPEN_SOURCE 700

#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT_NO 5000
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024

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
#include <signal.h>

struct device {
   int fd;
   struct sockaddr_in addr;
   struct device *next;
};

// Function Prototypes

// Initializers
extern struct device *initialize_tcp_server(const char *ip_address, int port);
extern struct device *initialize_tcp_client(const char *ip_address, int port);
extern struct device *accept_client(struct device *server);
extern void initialize_signal_handler();

// Sending Messages
extern void message_client(struct device *client, const char *message_bufffer);
extern void message_client_fd(int client_fd, const char *message_buffer);
extern void broadcast_message(const char *message);

// Device Management
extern struct device *get_device_from_fd(int fd);
extern struct device *get_prev_client_in_list(struct device *client);
extern struct device *get_next_client_in_list(struct device *client);

// Cleanup
extern void cleanup_client(struct device *client);
extern void cleanup_server(struct device *server);
extern void cleanup_all_clients();
extern void handle_signal(int sig);


// Global Variables
extern pthread_rwlock_t device_list_lock;
extern struct device *device_list_head;
extern struct device *tcp_server;
extern struct device *tcp_client;

#endif
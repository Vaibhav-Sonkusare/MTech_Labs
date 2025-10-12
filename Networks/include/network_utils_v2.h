// network_utils.h

#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H 

#define _XOPEN_SOURCE 700

#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT_NO 5000
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024
#define CLIENT_NAME_LEN 256

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
   char name[CLIENT_NAME_LEN];
   struct device *next;
};

// Function Prototypes

// Initializers
extern struct device *initialize_tcp_server(const char *ip_address, int port);
extern struct device *initialize_tcp_client(const char *ip_address, int port);
extern struct device *accept_client(struct device *server);
extern void initialize_signal_handler();

// Server Side Function
// Concurrent client management
extern int concurrently_handle_clients_with_handler(void *(*__start_routine)(void *), int log_level);

// Sending Messages
extern void message_device(struct device *client, const char *message_bufffer);
extern void message_device_fd(int client_fd, const char *message_buffer);
extern void broadcast_message(const char *message);
extern void message_device_formatted(struct device *client, const char *fmt, ...);

// Get message from device
extern ssize_t receive_message(struct device *client, char *buffer, size_t size);

// Device Management
extern struct device *get_device_from_fd(int fd);
extern struct device *get_prev_client_in_list(struct device *client);
extern struct device *get_next_client_in_list(struct device *client);
extern char *get_client_ip(struct device *client);
extern uint16_t get_client_port(struct device *client);

// Cleanup
extern void cleanup_client(struct device *client);
extern void cleanup_server(struct device *server);
extern void cleanup_all_clients();
extern void handle_signal(int sig);

// Other extra Utitility Functions
int max(int a, int b);

// Global Variables
extern pthread_rwlock_t device_list_lock;
extern struct device *device_list_head;
extern struct device *tcp_server;
extern struct device *tcp_client;

#endif




/**
 * @brief Send a formatted message to a client.
 *
 * This function formats a message using printf-style formatting and sends it
 * to the specified client using the `message_device()` function.
 * The message is copied into an internal stack buffer before sending.
 *
 * @param client Pointer to the client structure. Must not be NULL.
 * @param fmt    printf-style format string. Can include format specifiers
 *               like %s, %d, etc.
 * @param ...    Arguments corresponding to format specifiers in fmt.
 *
 * @return void
 *
 * @note The message is truncated if it exceeds BUFFER_SIZE - 1 characters.
 *       If truncation occurs, the function prints an error message to stderr
 *       and sets errno to EMSGSIZE.
 *
 * @note If an encoding error occurs during formatting (e.g., invalid
 *       multibyte sequences), the function prints an error message to stderr
 *       and sets errno to EILSEQ.
 *
 * @note The function is thread-safe because the internal buffer is allocated
 *       on the stack. However, sending messages on the same socket from
 *       multiple threads may result in interleaved messages.
 *
 * @example
 * send_formatted_message(client, "You will be solving %s paper.\nBest of Luck!\n\n",
 *                        question_paper->paper_name);
 * send_formatted_message(client, "Welcome to the server!\n");
 */
// extern void message_device_formatted(struct device *client, const char *fmt, ...);
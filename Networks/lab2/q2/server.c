/* Develop a mini chat application using socket programming, where clients
 * can communicate with each other through a centralized server—similar to
 * platforms like WhatsApp or Instagram. Each client's chat history must be
 * stored separately, and the server should have the ability to retrieve and
 * display the chat logs for any specific client upon request.
 */

#include <stdio.h>
#include <stdlib.h>             // for {m,c}alloc syscall
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>             // for close syscall
#include <ctype.h>              // for isspace(), isdigit(), etc.
#include <pthread.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024
#define MAX_STACK 1024
#define ERROR_EVALUATION -1234
#define CLIENT_NAME_LEN 256

struct device {
    int fd;
    struct sockaddr_in addr;
};

struct client_info {
    char name[CLIENT_NAME_LEN];
    struct device *dev;
    struct client_info *next;
};

// Client and server initialization and cleanup
struct device *initialize_server();
struct device *accept_client(struct device *);
void cleanup_device(struct device *);

// Message handling beteen clients
void register_client(char *, struct device *);
void save_registered_clients();
// void load_registered_clients();
void save_message(struct device *, struct device *, char *);
char **get_chat_history(struct device *, struct device *);
void *handle_client_communication(void *);

struct client_info *client_list_head = NULL;

int main(int argc, char **argv) {
    struct device *server = initialize_server();
    printf("Server initialized.\n");

    // Here server forks a new process for each client connection
    // Child process handles client registration and messaging
    // Parent process goes back to accept new client connections
    while (1) {
        struct device *client = accept_client(server);
        if (client == NULL) continue;
        printf("Client connected.\n");
        // Get client name for registration
        char *name_buffer = calloc(CLIENT_NAME_LEN, sizeof(char));
        if (name_buffer == NULL) {
            perror("main: calloc failed");
            cleanup_device(client);
            continue;
        }
        ssize_t bytes_read = read(client->fd, name_buffer, CLIENT_NAME_LEN);
        if (bytes_read <= 0) {
            perror("main: read client name failed");
            free(name_buffer);
            cleanup_device(client);
            continue;
        }
        name_buffer[bytes_read - 1] = '\0'; // Remove newline character
        printf("Registering client with name: %s\n", name_buffer);
        register_client(name_buffer, client);
        save_registered_clients();
        free(name_buffer);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_communication, client) != 0) {
            perror("main: pthread_create failed");
            continue;
        } 

    }

    return 0;
}

struct device *initialize_server() {
    struct device *server = calloc(1, sizeof(struct device));
    if (server == NULL) {
        perror("initilize_server: calloc failed");
        exit(EXIT_FAILURE);
    }

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->fd == -1) {
        perror("initialize_connection: Socket Creation Failed!");
        exit(EXIT_FAILURE);
    }

    server->addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDRESS, &server->addr.sin_addr);
    server->addr.sin_port = htons(PORT_NO);
    
    if (bind(server->fd, (struct sockaddr *) &server->addr, sizeof(server->addr)) == -1) {
        perror("initialize_connection: bind Failed!");
        close(server->fd);
        free(server);
        exit(EXIT_FAILURE);
    }

    if (listen(server->fd, LISTEN_BACKLOG) == -1) {
        perror("initialize_connection: listen Failed!");
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
    char *data = calloc(BUFFER_SIZE, sizeof(char));
    if (data == NULL) {
        perror("accept_client: calloc failed");
        free(client);
        return NULL;
    }

    socklen_t client_sockaddr_in_size = sizeof(client->addr);
    client->fd = accept(server->fd, (struct sockaddr*) &client->addr, &client_sockaddr_in_size);

    if (client->fd == -1) {
        perror("Error Connecting Client");
        free(client);
        return NULL;
    }

    return client;
}


// Close the client file descriptor by using close syscall and free associated memory
void cleanup_device(struct device *client) {
	struct client_info *curr = client_list_head;
	if (curr != NULL && curr->dev == client) {
		client_list_head = client_list_head->next;
	} else {
		while (curr->next != NULL) {
			if (curr->next->dev == client) {
				struct client_info *tmp = curr->next;
				curr->next = curr->next->next;
				free(curr->next);
				break;
			}
			curr = curr->next;
		}
	}

    if (close(client->fd) == -1) {
        perror("cleanup_device: unable to close file descriptor");
    }
    free(client);
}


void register_client(char *client_name, struct device *dev) {
    if (client_list_head == NULL) {
        // First client
        struct client_info *new_client = calloc(1, sizeof(struct client_info));
        if (new_client == NULL) {
            perror("register_client: calloc failed");
            return;
        }
        new_client->dev = dev;
        strncpy(new_client->name, client_name, CLIENT_NAME_LEN - 1);
        new_client->name[CLIENT_NAME_LEN - 1] = '\0'; // Ensure null-termination
        new_client->next = NULL;
        client_list_head = new_client;
    } else {
        // Add to existing list
        struct client_info *curr = client_list_head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        struct client_info *new_client = calloc(1, sizeof(struct client_info));
        if (new_client == NULL) {
            perror("register_client: calloc failed");
            return;
        }
        new_client->dev = dev;
        strncpy(new_client->name, client_name, CLIENT_NAME_LEN - 1);
        new_client->name[CLIENT_NAME_LEN - 1] = '\0'; // Ensure null-termination
        new_client->next = NULL;
        curr->next = new_client;
    }
}

void save_registered_clients() {
    FILE *file = fopen("registered_clients.txt", "w");
    if (file == NULL) {
        perror("save_registered_clients: fopen failed");
        return;
    }

    struct client_info *curr = client_list_head;
    while (curr != NULL) {
        fprintf(file, "%s\n", curr->name);
        curr = curr->next;
    }

    fclose(file);
}

// void load_registered_clients() {
//     FILE *file = fopen("registered_clients.txt", "r");
//     if (file == NULL) {
//         // No registered clients file, nothing to load
//         return;
//     }

//     char line[CLIENT_NAME_LEN];
//     while (fgets(line, sizeof(line), file)) {
//         // Remove newline character
//         line[strcspn(line, "\n")] = 0;
//         // Register client with NULL device (to be updated upon actual connection)
//         register_client(line, NULL);
//     }

//     fclose(file);
// }

void save_message(struct device *client__1, struct device *client_2, char *message) {
    // Implement file based chat history storage
    // History file named as <client1_name>_<client2_name>.txt
    // Each message prefixed with sender's name

    struct client_info *c1 = client_list_head;
    while (c1 != NULL && c1->dev != client__1) {
        c1 = c1->next;
    }
    struct client_info *c2 = client_list_head;
    while (c2 != NULL && c2->dev != client_2) {
        c2 = c2->next;
    }

    if (c1 == NULL || c2 == NULL) {
        fprintf(stderr, "save_message: One of the clients not registered.\n");
        return;
    }

    char filename[2 * CLIENT_NAME_LEN + 6];
    snprintf(filename, sizeof(filename), "%s_%s.txt", c1->name, c2->name);
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("save_message: fopen failed");
        return;
    }
    fprintf(file, "%s\n", message);
    fclose(file);
}

char **get_chat_history(struct device *client_1, struct device *client_2) {
    struct client_info *c1 = client_list_head;
    while (c1 != NULL && c1->dev != client_1) {
        c1 = c1->next;
    }
    struct client_info *c2 = client_list_head;
    while (c2 != NULL && c2->dev != client_2) {
        c2 = c2->next;
    }

    if (c1 == NULL || c2 == NULL) {
        fprintf(stderr, "get_chat_history: One of the clients not registered.\n");
        return NULL;
    }

    char filename_1[2 * CLIENT_NAME_LEN + 6];
    snprintf(filename_1, sizeof(filename_1), "%s_%s.txt", c1->name, c2->name);
    FILE *file = fopen(filename_1, "r");
    if (file == NULL) {
        perror("get_chat_history: fopen failed");
        return NULL;
    }

    char **history = calloc(BUFFER_SIZE, sizeof(char *));
    if (history == NULL) {
        perror("get_chat_history: calloc failed");
        fclose(file);
        return NULL;
    }

    char line[BUFFER_SIZE];
    int count = 0;
    while (fgets(line, sizeof(line), file) && count < BUFFER_SIZE - 1) {
        line[strcspn(line, "\n")] = 0; // Remove newline
        history[count] = strdup(line);
        if (history[count] == NULL) {
            perror("get_chat_history: strdup failed");
            break;
        }
        count++;
    }
    history[count] = NULL; // Null-terminate the array

    fclose(file);
    return history;
}

void *handle_client_communication(void *arg) {
    // To be implemented: Handle messaging between clients
    // This function will be called as a new thread using pthread

    struct device *client = (struct device *) arg;
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_read = read(client->fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            perror("hadle_incoming_client_message: read failed or client disconnected");
            cleanup_device(client);
            pthread_exit(NULL);
        }
        buffer[bytes_read] = '\0';

        // Expected message format: "TO:<recipient_name>:<message>"
        if (strncmp(buffer, "TO:", 3) == 0) {
            char *recipient_name = buffer + 3;
            char *msg_sep = strchr(recipient_name, ':');
            if (!msg_sep) {
                fprintf(stderr, "hadle_incoming_client_message: Invalid message format\n");
                continue;
            }
            *msg_sep = '\0';
            char *message = msg_sep + 1;

            // Find recipient in client list
            struct client_info *recipient = client_list_head;
            while (recipient) {
                if (strcmp(recipient->name, recipient_name) == 0) break;
                recipient = recipient->next;
            }
            if (!recipient || !recipient->dev) {
                fprintf(stderr, "hadle_incoming_client_message: Recipient not found\n");
                continue;
            }
            
            // Find sender in client list
            struct client_info *sender_info = client_list_head;
            while (sender_info) {
                if (sender_info->dev == client) break;
                sender_info = sender_info->next;
            }
            if (!sender_info || !sender_info->dev) {
                fprintf(stderr, "hadle_incoming_client_message: sender_info not found\n");
                continue;
            }

            // Save message to chat history
            save_message(client, recipient->dev, message);
            save_message(recipient->dev, client, message);

            // Forward message to recipient
            char forward_msg[BUFFER_SIZE];
            snprintf(forward_msg, sizeof(forward_msg), "FROM:%s:%s\n", sender_info->name, message);
            write(recipient->dev->fd, forward_msg, strlen(forward_msg));
        } else if (strncmp(buffer, "HISTORY:", 8) == 0) {
            char *peer_name = buffer + 8;
            peer_name[strcspn(peer_name, "\n")] = 0;
            struct client_info *peer = client_list_head;
            while (peer) {
                if (strcmp(peer->name, peer_name) == 0) break;
                peer = peer->next;
            }
            if (!peer || !peer->dev) {
                fprintf(stderr, "hadle_incoming_client_message: Peer not found\n");
                continue;
            }
            char **history = get_chat_history(client, peer->dev);
            if (history) {
                for (int i = 0; history[i]; i++) {
                    write(client->fd, history[i], strlen(history[i]));
                    write(client->fd, "\n", 1);
                    free(history[i]);
                }
                free(history);
            }
        } else if (strncmp(buffer, "CLIENTS", 7) == 0) {
            // Send list of registered clients
            struct client_info *curr = client_list_head;
            while (curr) {
                write(client->fd, curr->name, strlen(curr->name));
                write(client->fd, "\n", 1);
                curr = curr->next;
            }
        } else {
            fprintf(stderr, "hadle_incoming_client_message: Unknown command\n");
        }
    }

}

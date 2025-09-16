// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define NAME_LEN 32

struct message {
    char from[NAME_LEN];
    char text[BUFFER_SIZE];
    struct message *next;
};

struct chat_history {
    char peer_name[NAME_LEN];
    struct message *head;
};

struct client {
    int fd;
    char name[NAME_LEN];
    struct chat_history chats[MAX_CLIENTS];
    int chat_count;
};

struct client clients[MAX_CLIENTS];
int client_count = 0;

void send_to_client(int client_fd, const char *msg) {
    write(client_fd, msg, strlen(msg));
}

void broadcast_client_list() {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "Connected clients:\n");
    for (int i = 0; i < client_count; ++i) {
        char line[64];
        snprintf(line, sizeof(line), "%d. %s\n", i + 1, clients[i].name);
        strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
    }
    for (int i = 0; i < client_count; ++i) {
        send_to_client(clients[i].fd, buffer);
    }
}

struct client* find_client_by_name(const char *name) {
    for (int i = 0; i < client_count; ++i) {
        if (strcmp(clients[i].name, name) == 0) {
            return &clients[i];
        }
    }
    return NULL;
}

struct client* find_client_by_index(int index) {
    if (index >= 0 && index < client_count) {
        return &clients[index];
    }
    return NULL;
}

void add_message(struct client *sender, struct client *receiver, const char *msg_text) {
    struct message *msg = malloc(sizeof(struct message));
    strcpy(msg->from, sender->name);
    strncpy(msg->text, msg_text, BUFFER_SIZE);
    msg->next = NULL;

    // Find or add chat history with receiver
    struct chat_history *hist = NULL;
    for (int i = 0; i < sender->chat_count; ++i) {
        if (strcmp(sender->chats[i].peer_name, receiver->name) == 0) {
            hist = &sender->chats[i];
            break;
        }
    }
    if (!hist && sender->chat_count < MAX_CLIENTS) {
        hist = &sender->chats[sender->chat_count++];
        strcpy(hist->peer_name, receiver->name);
        hist->head = NULL;
    }

    if (hist) {
        msg->next = hist->head;
        hist->head = msg;
    }
}

void handle_client(int client_index) {
    char buffer[BUFFER_SIZE];
    struct client *cli = &clients[client_index];

    send_to_client(cli->fd, "Enter the client number you want to chat with:\n");

    // Broadcast list of clients
    broadcast_client_list();

    ssize_t read_size = read(cli->fd, buffer, BUFFER_SIZE - 1);
    if (read_size <= 0) {
        close(cli->fd);
        return;
    }
    buffer[read_size] = '\0';
    int target_index = atoi(buffer) - 1;
    struct client *target = find_client_by_index(target_index);

    if (!target || target == cli) {
        send_to_client(cli->fd, "Invalid client selection.\n");
        return;
    }

    send_to_client(cli->fd, "Enter your message:\n");
    read_size = read(cli->fd, buffer, BUFFER_SIZE - 1);
    if (read_size <= 0) {
        close(cli->fd);
        return;
    }
    buffer[read_size] = '\0';

    // Store message in both clients' histories
    add_message(cli, target, buffer);
    add_message(target, cli, buffer);

    char notify[BUFFER_SIZE];
    snprintf(notify, sizeof(notify), "Message from %s: %s\n", cli->name, buffer);
    send_to_client(target->fd, notify);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept failed");
            continue;
        }

        if (client_count >= MAX_CLIENTS) {
            send_to_client(client_fd, "Server is full.\n");
            close(client_fd);
            continue;
        }

        send_to_client(client_fd, "Enter your username:\n");
        char name[NAME_LEN];
        ssize_t read_size = read(client_fd, name, NAME_LEN - 1);
        if (read_size <= 0) {
            close(client_fd);
            continue;
        }
        name[read_size] = '\0';
        name[strcspn(name, "\n")] = '\0'; // remove newline

        struct client *existing = find_client_by_name(name);
        if (existing) {
            send_to_client(client_fd, "Username already exists.\n");
            close(client_fd);
            continue;
        }

        struct client *cli = &clients[client_count++];
        cli->fd = client_fd;
        strncpy(cli->name, name, NAME_LEN);
        cli->chat_count = 0;

        send_to_client(client_fd, "Welcome to the chat!\n");

        handle_client(client_count - 1);

        close(client_fd);
        printf("Client %s disconnected\n", name);
    }

    close(server_fd);
    return 0;
}

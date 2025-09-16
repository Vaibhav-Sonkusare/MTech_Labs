#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define BUFFER_SIZE 1024
#define CLIENT_NAME_LEN 256

int sock_fd;

// Thread to listen for messages from server
void *listen_to_server(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_read = read(sock_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            printf("Disconnected from server.\n");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;

    // Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_NO);
    inet_pton(AF_INET, IP_ADDRESS, &server_addr.sin_addr);

    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Get and send client name
    char name[CLIENT_NAME_LEN];
    printf("Enter your name: ");
    fgets(name, CLIENT_NAME_LEN - 1, stdin);
    name[strcspn(name, "\n") + 1] = '\0'; // remove newline
    name[CLIENT_NAME_LEN - 1] = '\0';
    write(sock_fd, name, strlen(name));

    // Start thread to listen for messages
    pthread_t tid;
    if (pthread_create(&tid, NULL, listen_to_server, NULL) != 0) {
        perror("pthread_create failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Main loop to send commands to server
    char buffer[BUFFER_SIZE];
    while (1) {
        printf("Enter command (TO:<name>:<msg> / HISTORY: / CLIENTS):\n");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0; // remove newline
        if (strlen(buffer) == 0) continue;

        // Send to server
        ssize_t bytes_sent = write(sock_fd, buffer, strlen(buffer));
        if (bytes_sent == -1) {
            perror("write failed");
            break;
        }
    }

    close(sock_fd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define BUFFER_SIZE 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_NO);
    inet_pton(AF_INET, IP_ADDRESS, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char username[BUFFER_SIZE];
    printf("Enter your username: ");
    fgets(username, BUFFER_SIZE, stdin);
    username[strcspn(username, "\n")] = '\0';

    send(sock, username, strlen(username), 0);

    char buffer[BUFFER_SIZE];
    ssize_t n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s\n", buffer);
    }

    while (1) {
        printf("Enter message (or 'history'/'exit'): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        send(sock, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "exit") == 0) {
            printf("Disconnected.\n");
            break;
        }

        // Receive response
        n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            printf("%s\n", buffer);
        }
    }

    close(sock);
    return 0;
}

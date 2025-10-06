// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 1024

int main(int argc, char *argv[]) {
    int server_fd, clients[3], addrlen, i;
    struct sockaddr_in address;
    char buffer[MAX];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);

    if (argc == 1) {
        printf("Server ready. Waiting for 3 clients (Sender, Receiver, Error Client)...\n");
        for (i = 0; i < 3; i++) {
            clients[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (clients[i] < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("Client %d connected.\n", i+1);
        }
    } else {
        printf("Server ready. Waiting for 2 clients (Sender, Receiver)...\n");
        for (i = 0; i < 2; i++) {
            clients[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (clients[i] < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("Client %d connected.\n", i+1);
        }
    }

    // --- Sender -> Error Client or Receiver ---
    while (1) {
        memset(buffer, 0, MAX);
        read(clients[0], buffer, MAX);
        printf("Server got from Sender: %s\n", buffer);

        if (argc == 1) {
            // 3-client mode (with Error Client)
            write(clients[2], buffer, strlen(buffer));
            printf("Server forwarded data to Error Client\n");

            memset(buffer, 0, MAX);
            read(clients[2], buffer, MAX);
            printf("Server got from Error Client: %s\n", buffer);

            write(clients[1], buffer, strlen(buffer));
            printf("Server forwarded data to Receiver\n");
        } else {
            // 2-client mode (direct to Receiver)
            write(clients[1], buffer, strlen(buffer));
            printf("Server forwarded data directly to Receiver\n");
        }

        // --- Receiver -> Sender (ACK) ---
        memset(buffer, 0, MAX);
        read(clients[1], buffer, MAX);
        printf("Server got ACK from Receiver: %s\n", buffer);

        write(clients[0], buffer, strlen(buffer));
        printf("Server sent ACK to Sender\n");

        if (strncmp(buffer, "Error", 5) != 0) {
            break;
        }
    }

    close(server_fd);
    return 0;
}

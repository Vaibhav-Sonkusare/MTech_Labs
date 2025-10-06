// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 1024

int main(int argc, char *argv[]) {
    int server_fd, new_socket[3], addrlen, i;
    struct sockaddr_in address;
    char buffer[MAX];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    addrlen = sizeof(address);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server ready. Waiting for clients...\n");

    if (argc == 1) {
        // Expect 3 clients: sender, receiver, error_sender
        for (i = 0; i < 3; i++) {
            new_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket[i] < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("Client %d connected.\n", i+1);
        }
    } else {
        // Expect only 2 clients: sender, receiver
        for (i = 0; i < 2; i++) {
            new_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket[i] < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("Client %d connected.\n", i+1);
        }
    }

    // Expect sender to send first
    memset(buffer, 0, MAX);
    read(new_socket[0], buffer, MAX);
    printf("Server got data from Sender: %s\n", buffer);

    if (argc == 1) {
        // Full flow with Error Sender
        write(new_socket[2], buffer, strlen(buffer));
        printf("Server forwarded data to Error Sender\n");

        memset(buffer, 0, MAX);
        read(new_socket[2], buffer, MAX);
        printf("Server got modified data: %s\n", buffer);

        write(new_socket[1], buffer, strlen(buffer));
        printf("Server forwarded data to Receiver\n");
    } else {
        // Direct flow: Sender -> Receiver
        write(new_socket[1], buffer, strlen(buffer));
        printf("Server forwarded data directly to Receiver\n");
    }

    // Get ACK from Receiver
    memset(buffer, 0, MAX);
    read(new_socket[1], buffer, MAX);
    printf("Server got ACK from Receiver: %s\n", buffer);

    // Send ACK to Sender
    write(new_socket[0], buffer, strlen(buffer));
    printf("Server sent ACK to Sender\n");

    close(server_fd);
    return 0;
}

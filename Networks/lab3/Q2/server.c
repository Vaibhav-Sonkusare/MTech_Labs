// gcc server.c -o server
// ./server <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENTS 3
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int server_fd, client_fd[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    fd_set readfds;
    socklen_t addr_len;
    char buffer[BUF_SIZE];

    // Init
    for (int i=0; i<MAX_CLIENTS; i++) client_fd[i] = -1;

    // Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 3);

    printf("Server listening on port %d...\n", port);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int i=0; i<MAX_CLIENTS; i++) {
            if (client_fd[i] > 0)
                FD_SET(client_fd[i], &readfds);
            if (client_fd[i] > max_sd)
                max_sd = client_fd[i];
        }

        select(max_sd+1, &readfds, NULL, NULL, NULL);

        // New connection
        if (FD_ISSET(server_fd, &readfds)) {
            addr_len = sizeof(client_addr);
            int new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            printf("New client connected!\n");
            for (int i=0; i<MAX_CLIENTS; i++) {
                if (client_fd[i] == -1) {
                    client_fd[i] = new_socket;
                    break;
                }
            }
        }

        // Forward messages
        for (int i=0; i<MAX_CLIENTS; i++) {
            if (client_fd[i] != -1 && FD_ISSET(client_fd[i], &readfds)) {
                int val = read(client_fd[i], buffer, BUF_SIZE);
                if (val <= 0) {
                    close(client_fd[i]);
                    client_fd[i] = -1;
                } else {
                    buffer[val] = '\0';
                    printf("Forwarding: %s\n", buffer);

                    // Simple round-robin forwarding:
                    // 0=Sender → 1=Error → 2=Receiver → back to Sender
                    if (i == 0) send(client_fd[1], buffer, strlen(buffer), 0);
                    else if (i == 1) send(client_fd[2], buffer, strlen(buffer), 0);
                    else if (i == 2) send(client_fd[0], buffer, strlen(buffer), 0);
                }
            }
        }
    }
}


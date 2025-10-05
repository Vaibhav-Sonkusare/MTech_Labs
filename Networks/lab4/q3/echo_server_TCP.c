// echo_server_TCP.c

#include "../../include/network_utils.h"

char *reverse_strign_case(char *str) {
    if (str == NULL) {
        return NULL;
    }

    size_t len = strlen(str);
    if (len <= 0) {
        return NULL;
    } else if (len >= BUFFER_SIZE - 1) {
        len = BUFFER_SIZE - 2; // leave space for null terminator
    }
    for (size_t i = 0; i < len; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = str[i] - 'a' + 'A';
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = str[i] - 'A' + 'a';
        }
    }
    str[len] = '\0';

    return str;
}

int main () {
    tcp_server = initialize_tcp_server(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);
    if (tcp_server == NULL) {
        return -1;
    }

    printf("TCP Echo Server listening on %s:%d\n", DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    while (1) {
        printf("\n\nWaiting for client connection...\n");

        struct device *client = accept_client(tcp_server);
        if (client == NULL) {
            cleanup_server(tcp_server);
            return -1;
        }
        printf("Client Connected!\n");
        printf("Client IP: %s, Port: %d\n", inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));

        printf("Waiting for message from client...\n");
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        ssize_t bytes_received = recv(client->fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received < 0) {
            perror("recv failed");
        } else if (bytes_received == 0) {
            fprintf(stderr, "Client disconnected\n");
        } else {
            buffer[bytes_received] = '\0'; // Null-terminate the received string
            printf("Received message: %s\n", buffer);

            // Reverse the case of the string
            char *modified_message = reverse_strign_case(buffer);
            if (modified_message == NULL) {
                fprintf(stderr, "Error reversing string case\n");
                cleanup_client(client);
                continue;
            }

            // Echo the message back to the client 
            message_client(client, modified_message);

            printf("Echoed message back to client: %s\n", modified_message);
            printf("Disconnecting client...\n");
        }
        cleanup_client(client);
    }

    return 0;
}
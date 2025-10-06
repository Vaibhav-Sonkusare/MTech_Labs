// client_TCP.c

#include "../../include/network_utils.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "server ip and port not provided!\n");
        fprintf(stderr, "Usage: %s <SERVER_IP_ADDRESS> <SERVER_PORT>", argv[0]);
        return -1;
    }

    tcp_client = initialize_tcp_client(argv[1], atoi(argv[2]));
    if (tcp_client == NULL) {
        return -1;
    }

    printf("Connected to TCP Server at %s:%d\n", DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    // Get and send message to server
    char message[BUFFER_SIZE];
    printf("Enter message to send to server: ");
    if (fgets(message, BUFFER_SIZE - 1, stdin) == NULL) {
        fprintf(stderr, "Error reading input\n");
        cleanup_client(tcp_client);
        return -1;
    }
    message[strcspn(message, "\n")] = '\0'; // remove newline
    message[BUFFER_SIZE - 1] = '\0';
    message_client(tcp_client, message);
    printf("Message sent to server: %s\n", message);

    printf("Waiting for response from server...\n");
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    ssize_t bytes_received = recv(tcp_client->fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received < 0) {
        perror("recv failed");
    } else if (bytes_received == 0) {
        fprintf(stderr, "Server disconnected\n");
    } else {
        buffer[bytes_received] = '\0'; // Null-terminate the received string
        printf("Received response from server: %s\n", buffer);
    }

    printf("Disconnecting from server...\n");
    cleanup_client(tcp_client);
    return 0;
}
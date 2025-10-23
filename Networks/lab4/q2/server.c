#include "../../include/network_utils.h"

int main() {
    tcp_server = initialize_tcp_server(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    struct device *client = calloc(1, sizeof(struct device));
    if (client == NULL) {
        perror("accept_client: calloc failed");
        return -1;
    }

    socklen_t client_sockaddr_in_size = sizeof(client->addr);
    client->fd = accept(tcp_server->fd, (struct sockaddr*) &client->addr, &client_sockaddr_in_size);
    if (client->fd == -1) {
        perror("accept_client: accept failed");
        free(client);
        return -2;
    }

    // struct device *cli = accept_client(tcp_server);
    printf("Client connected!\n");

    // message_client(cli, "Hi port scanner!!!");
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    strcpy(buffer, "Hi, I am a custom server!");
    send(client->fd, buffer, strlen(buffer), 0);

    printf("loop start\n");
    long temp = 100000000000;
    while (temp--); 
    printf("loop end\n");


    cleanup_client(client);
    cleanup_server(tcp_server);

    return 0;
}
#include "../../include/network_utils_v2.h"

// Get message from client with variable size
extern ssize_t receive_message_custom_len(struct device *client, char *payload, size_t max_size, uint16_t *type) {
    if (client == NULL || payload == NULL || max_size <= 0) {
        errno = EINVAL;
        return -1;
    }

    struct MessageHeader header;
    ssize_t header_bytes = recv(client->fd, &header, sizeof(header), 0);
    if (header_bytes <= 0) {
        return header_bytes;
    }

    *type = ntohs(header.type);
    uint16_t lenght = header.length;

    if (lenght >= max_size) {
        if (debug > 1) {
            fprintf(stderr, "receive_message_custom_len: message length (%d) > max_size (%ld)", lenght, max_size);
        }
        lenght = max_size - 1;
    }

    memset(payload, '\0', max_size);
    if (lenght > 0) {
        // char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client->fd, payload, lenght, 0);
        if (bytes_received <= 0) {
            return bytes_received;
        }
        
        // strncpy(payload, buffer, lenght);
        payload[bytes_received] = '\0';
		if (debug > 2) {
        	fprintf(stderr, "%s", payload);
		}
        return bytes_received;
    } else {
        return lenght;
    }
}

int main() {
    tcp_client = initialize_tcp_client(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    char buff[BUFFER_SIZE];
    uint16_t _type;
    receive_message_custom_len(tcp_client, buff, BUFFER_SIZE, &_type);

    printf("Message received: %s\n", buff);

    // fclose(tcp_client->fd);
    return 0;
}
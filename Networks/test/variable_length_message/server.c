#include "../../include/network_utils_v2.h"

// Sending Messages with variable size
int message_device_custom_len(struct device *client, uint16_t type, const char *payload, size_t payload_size) {
    if (client == NULL) {
        errno = EINVAL;
        return -EINVAL;
    }

    struct MessageHeader header;
    header.type = htons(type);
    header.length = payload_size;

    // send header
    int ret_val = send(client->fd, &header, sizeof(header), 0);
    if (ret_val < 0) {
        return ret_val;
    }

    // send payload if any
	if (debug > 2) {
    	fprintf(stderr, "payload = %s^^^%ld^^\n", payload, payload_size);
	}
    if (payload && payload_size > 0) {
        // char buffer[BUFFER_SIZE];
        // strncpy(buffer, payload, BUFFER_SIZE);
        ret_val = send(client->fd, payload, payload_size, 0);
        if (ret_val < 0) {
            return ret_val;
        }
    }

    return ret_val;
}

int main () {
    tcp_server = initialize_tcp_server(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    struct device *client = accept_client(tcp_server);

    char *message = "This is the custom message<END>";
    message_device_custom_len(client, MESSAGE_TYPE_NORMAL, message, strlen(message));

    cleanup_server(tcp_server);

    return 0;
}
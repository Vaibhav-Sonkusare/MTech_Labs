#include "../../include/network_utils_v2.h"
#include "question.h"
#include "paper.h"

int main() {
    tcp_client = initialize_tcp_client(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    char buffer[BUFFER_SIZE];
    uint16_t msg_type;

    while (1) {
        ssize_t msg_size = receive_message(tcp_client, buffer, BUFFER_SIZE, &msg_type);
        if (msg_type == MESSAGE_TYPE_WAITINP) {
			if (debug > 1) {
            	printf("\nreceived MESSAGE_TYPE_WAITINP\n");
			}
            memset(buffer, '\0', BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            message_device_formatted(tcp_client, MESSAGE_TYPE_NORMAL, buffer);
        } else if (msg_type == MESSAGE_TYPE_CLOSURE) {
			if (debug > 1) {
            	printf("\nreceived MESSAGE_TYPE_CLOSURE\n");
			}
            cleanup_client(tcp_client);
            exit(EXIT_SUCCESS);
        } else if (msg_size <= 0) {
			if (debug > 1) {
            	printf("\nreceived msg_size <= 0\n");
			}
            perror("receive_message: error receiving message or server disconnected!\n");
            cleanup_client(tcp_client);
            exit(EXIT_FAILURE);
        } else if (msg_type == MESSAGE_TYPE_NORMAL) {
			if (debug > 1) {
            	printf("\nreceived MESSAGE_TYPE_NORMAL\n");
			}
            printf("%s", buffer);
        } 
    }
    
    return -1;      // flow will never reach here
}

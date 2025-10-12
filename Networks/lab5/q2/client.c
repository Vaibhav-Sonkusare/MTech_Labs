#include "../../include/network_utils.h"
#include "question.h"
#include "paper.h"

int main() {
    tcp_client = initialize_tcp_client(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, '\0', BUFFER_SIZE);
        ssize_t msg_size = receive_message(tcp_client, buffer, BUFFER_SIZE);

        if (msg_size < 0) {
            perror("receive_message: error");
            cleanup_client(tcp_client);
            exit(EXIT_FAILURE);
        } else if {
            
        }
    }
    
}
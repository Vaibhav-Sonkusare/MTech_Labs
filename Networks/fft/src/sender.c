#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../include/protocol.h"
#include "../include/net.h"

typedef enum {
    S_IDLE,
    S_HDR_SENT,
    S_DONE
} sender_state_t;

int main(int argc, char** argv)
{
    if (argc != 4) {
        printf("Usage: %s <receiver_ip> <port> <filename>\n", argv[0]);
        return 1;
    }

    const char* receiver_ip = argv[1];
    uint16_t port = atoi(argv[2]);
    const char* filename = argv[3];

    /* ---- Prepare socket ---- */
    int sockfd = create_udp_socket();
    if (sockfd < 0) return 1;

    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
    inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr);

    /* ---- Prepare FILE_HDR ---- */
    pkt_file_hdr_t hdr;
    uint64_t fake_file_size = 12345;  // For now, no file reading yet
    uint16_t fname_len = strlen(filename);

    build_pkt_file_hdr(&hdr, filename, fake_file_size, fname_len);

    /* ---- FSM ---- */
    sender_state_t state = S_IDLE;

    while (state != S_DONE) {

        switch (state) {

            case S_IDLE:
                printf("[Sender] Sending FILE_HDR...\n");

                udp_send(sockfd, &hdr, sizeof(hdr), &receiver_addr);

                state = S_HDR_SENT;
                break;

            case S_HDR_SENT: {
                printf("[Sender] Waiting for FILE_HDR_ACK...\n");

                pkt_file_hdr_ack_t ack;
                struct sockaddr_in from;
                ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);

                if (n > 0 && ack.type == PKT_FILE_HDR_ACK) {
                    printf("[Sender] Received ACK! Status=%u\n", ack.status);
                    state = S_DONE;
                }
                break;
            }

            default:
                state = S_DONE;
        }
    }

    printf("[Sender] Done.\n");
    close(sockfd);
    return 0;
}

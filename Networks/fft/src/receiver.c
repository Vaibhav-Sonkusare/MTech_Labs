#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../include/protocol.h"
#include "../include/net.h"

typedef enum {
    R_IDLE,
    R_HDR_RECV,
    R_WAIT_BLAST,
    R_DONE
} receiver_state_t;

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    printf("pkt_file_hdr_t size: %zu\n", sizeof(pkt_file_hdr_t));

    uint16_t port = atoi(argv[1]);

    /* ---- Prepare socket ---- */
    int sockfd = create_udp_socket();
    if (sockfd < 0) return 1;

    if (bind_udp_socket(sockfd, port) < 0) return 1;

    printf("[Receiver] Listening on port %u...\n", port);

    /* ---- FSM ---- */
    receiver_state_t state = R_IDLE;

    while (state != R_DONE) {

        switch (state) {

            case R_IDLE: {
                pkt_file_hdr_t hdr;
                struct sockaddr_in sender_addr;

                printf("[Receiver] Waiting for FILE_HDR...\n");

                ssize_t n = udp_recv(sockfd, &hdr, sizeof(hdr), &sender_addr);

                if (n > 0 && hdr.type == PKT_FILE_HDR) {
                    printf("[Receiver] Received FILE_HDR for file: %s\n", hdr.filename);
                    printf("[Receiver] File size: %lu\n", hdr.file_size);

                    state = R_HDR_RECV;

                    /* Store sender address for response */
                    udp_send(sockfd, NULL, 0, &sender_addr); // (not needed, but safe)
                    
                    /* Immediately send ACK */
                    pkt_file_hdr_ack_t ack;
                    build_pkt_file_hdr_ack(&ack, 0,
                                           hdr._max_rec_size,
                                           hdr._max_records_per_packet,
                                           hdr._max_packets_per_blast);

                    udp_send(sockfd, &ack, sizeof(ack), &sender_addr);
                    printf("[Receiver] Sent ACK.\n");

                    state = R_WAIT_BLAST;
                }
                break;
            }

            case R_WAIT_BLAST:
                /* We stop after ACK for this minimal version */
                printf("[Receiver] Done for minimal version.\n");
                state = R_DONE;
                break;

            default:
                state = R_DONE;
        }
    }

    close(sockfd);
    return 0;
}

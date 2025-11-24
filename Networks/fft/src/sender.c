#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "../include/protocol.h"
#include "../include/net.h"
#include "../include/record.h"
#include "../include/fileio.h"

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

    printf("pkt_file_hdr_t size: %zu\n", sizeof(pkt_file_hdr_t));

    const char* receiver_ip = argv[1];
    uint16_t port = (uint16_t)atoi(argv[2]);
    const char* filename = argv[3];

    /* ---- Read file & slice into records ---- */
    record_t *records = NULL;
    uint32_t nrec = 0;
    uint64_t file_size = 0;
    uint16_t rec_size = DEFAULT_RECORD_SIZE;

    if (read_file_to_records(filename, rec_size, &records, &nrec, &file_size) != 0) {
        fprintf(stderr, "Failed to read and slice file: %s\n", filename);
        return 1;
    }

    printf("[Sender] File '%s' size=%lu bytes -> %u records (rec_size=%u)\n",
           filename, file_size, nrec, rec_size);

    /* ---- Prepare socket ---- */
    int sockfd = create_udp_socket();
    if (sockfd < 0) {
        free_records(records, nrec);
        return 1;
    }

    struct sockaddr_in receiver_addr;
    memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
    inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr);

    /* ---- Prepare FILE_HDR ---- */
    pkt_file_hdr_t hdr;
    uint16_t fname_len = (uint16_t)strlen(filename);
    build_pkt_file_hdr(&hdr, filename, file_size, fname_len);

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

    /* cleanup */
    close(sockfd);
    free_records(records, nrec);

    printf("[Sender] Done.\n");
    return 0;
}

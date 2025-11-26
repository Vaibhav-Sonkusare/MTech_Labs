// src/sender.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "../include/protocol.h"
#include "../include/net.h"
#include "../include/record.h"
#include "../include/fileio.h"
#include "../include/blast.h"

typedef enum {
    S_IDLE,
    S_HDR_SENT,
    S_SENDING_BLASTS,
    S_WAIT_REC_MISS,
    S_LAST_BLAST_DONE,
    S_DONE
} sender_state_t;

/* Helper: send one blast packet (rebuilds pkt each time)
 * Now accepts n_packets so pkt.n_packets is set correctly before sending.
 * Returns number of bytes sent or -1 on error.
 */
static ssize_t send_blast_packet(int sockfd, const struct sockaddr_in *to,
                                 uint32_t blast_id, uint32_t packet_id,
                                 uint32_t n_packets, /* total packets in this blast */
                                 const record_t *records, uint32_t start_idx,
                                 uint32_t nrec_total, uint16_t max_records_per_packet)
{
    pkt_blast_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    /* build packet payload (record ids + data) */
    uint16_t nput = build_blast_packet(&pkt, blast_id, packet_id,
                                       records, start_idx, nrec_total,
                                       max_records_per_packet);
    /* set n_packets so receiver knows how many packets to expect in this blast */
    pkt.n_packets = (uint32_t)n_packets;

    /* sanity */
    pkt.type = PKT_BLAST_PACKET;
    pkt.packet_id = packet_id;
    pkt.n_records = nput;

    ssize_t sent = udp_send(sockfd, &pkt, sizeof(pkt), to);
    if (sent < 0) perror("udp_send blast pkt");
    return sent;
}

int main(int argc, char** argv)
{
    if (argc != 5) {
        printf("Usage: %s <receiver_ip> <port> <filename> <loss_rate>\n", argv[0]);
        return 1;
    }

    const char* receiver_ip = argv[1];
    uint16_t port = (uint16_t)atoi(argv[2]);
    const char* filename = argv[3];
    double loss_rate = atof(argv[4]);
    net_set_loss_rate(loss_rate);

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
    build_pkt_file_hdr(&hdr, filename, fname_len, file_size);

    uint16_t max_records_per_packet = hdr._max_records_per_packet;
    uint16_t max_packets_per_blast = hdr._max_packets_per_blast;
    if (max_records_per_packet == 0) max_records_per_packet = DEFAULT_RECORDS_PER_PACKET;
    if (max_packets_per_blast == 0) max_packets_per_blast = DEFAULT_PACKETS_PER_BLAST;

    uint32_t records_per_blast = (uint32_t)max_records_per_packet * (uint32_t)max_packets_per_blast;
    if (records_per_blast == 0) {
        fprintf(stderr, "Invalid blast sizing (zero)\n");
        close(sockfd);
        free_records(records, nrec);
        return 1;
    }

    uint32_t total_blasts = (nrec + records_per_blast - 1) / records_per_blast;

    /* ---- FSM ---- */
    sender_state_t state = S_IDLE;
    uint32_t blast_id = 1;

    /* variables used across states */
    uint32_t last_packets_in_blast = 0;
    uint32_t last_blast_start_index = 0;

    while (state != S_DONE) {

        switch (state) {

            case S_IDLE:
                printf("[Sender] Sending FILE_HDR...\n");
                if (udp_send(sockfd, &hdr, sizeof(hdr), &receiver_addr) < 0) {
                    perror("udp_send FILE_HDR");
                    state = S_DONE;
                    break;
                }
                state = S_HDR_SENT;
                break;

            case S_HDR_SENT: {
                printf("[Sender] Waiting for FILE_HDR_ACK...\n");

                pkt_file_hdr_ack_t ack;
                struct sockaddr_in from;

                int retries = 5;

                while (retries-- > 0) {

                    ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);

                    if (n > 0 && ack.type == PKT_FILE_HDR_ACK) {
                        printf("[Sender] Received ACK! Status=%u\n", ack.status);

                        /* Adjust parameters if needed */
                        if (ack.status != 0) {
                            if (ack._max_records_per_packet > 0)
                                max_records_per_packet = ack._max_records_per_packet;
                            if (ack._max_packets_per_blast > 0)
                                max_packets_per_blast = ack._max_packets_per_blast;

                            if (max_packets_per_blast > DEFAULT_PACKETS_PER_BLAST)
                                max_packets_per_blast = DEFAULT_PACKETS_PER_BLAST;

                            records_per_blast =
                                (uint32_t)max_records_per_packet *
                                (uint32_t)max_packets_per_blast;

                            total_blasts = (nrec + records_per_blast - 1) / records_per_blast;

                            printf("[Sender] Adjusted max_records_per_packet=%u, max_packets_per_blast=%u\n",
                                max_records_per_packet, max_packets_per_blast);
                        }

                        state = S_SENDING_BLASTS;
                        break;     // exit retry loop
                    }

                    /* Timeout or wrong packet → retransmit FILE_HDR */
                    printf("[Sender] No ACK (timeout or loss). Resending FILE_HDR... (%d retries left)\n",
                        retries);

                    if (udp_send(sockfd, &hdr, sizeof(hdr), &receiver_addr) < 0) {
                        perror("udp_send FILE_HDR");
                        state = S_DONE;
                        break;
                    }
                }

                if (state == S_HDR_SENT) {
                    /* Exhausted retries */
                    fprintf(stderr, "[Sender] ERROR: No FILE_HDR_ACK after retries. Aborting.\n");
                    state = S_DONE;
                }

                break;
            }

            case S_SENDING_BLASTS: {
                if (blast_id > total_blasts) {
                    state = S_LAST_BLAST_DONE;
                    break;
                }

                /* Compute this blast's range */
                uint32_t blast_start_rec_index = (blast_id - 1) * records_per_blast; /* 0-based index */
                uint32_t remain = (blast_start_rec_index >= nrec) ? 0 : (nrec - blast_start_rec_index);
                uint32_t packets_in_this_blast = (remain + max_records_per_packet - 1) / max_records_per_packet;
                if (packets_in_this_blast == 0) packets_in_this_blast = 1;

                if (packets_in_this_blast > max_packets_per_blast)
                    packets_in_this_blast = max_packets_per_blast;

                last_packets_in_blast = packets_in_this_blast;
                last_blast_start_index = blast_start_rec_index;

                printf("[Sender] Sending blast %u: records [%u .. %u], packets=%u\n",
                       blast_id, blast_start_rec_index + 1,
                       (uint32_t)(blast_start_rec_index + remain), (unsigned)packets_in_this_blast);

                /* Send all packets that belong to this blast (packet_id 0..packets_in_this_blast-1) */
                for (uint32_t pkt_idx = 0; pkt_idx < packets_in_this_blast; ++pkt_idx) {
                    uint32_t rec_start = blast_start_rec_index + pkt_idx * max_records_per_packet;
                    ssize_t sent = send_blast_packet(sockfd, &receiver_addr,
                                                     blast_id, pkt_idx,
                                                     packets_in_this_blast,
                                                     records, rec_start, nrec,
                                                     max_records_per_packet);
                    if (sent < 0) {
                        fprintf(stderr, "[Sender] Failed to send blast packet %u/%u for blast %u\n",
                                (unsigned)pkt_idx, (unsigned)packets_in_this_blast, (unsigned)blast_id);
                    }
                }

                /* After sending the blast packets, send IS_BLAST_OVER and move to wait for REC_MISS */
                pkt_is_blast_over_t iso;
                build_pkt_is_blast_over(&iso, blast_id, packets_in_this_blast);
                if (udp_send(sockfd, &iso, sizeof(iso), &receiver_addr) < 0) {
                    perror("udp_send IS_BLAST_OVER");
                } else {
                    printf("[Sender] Sent IS_BLAST_OVER (blast=%u, packets=%u)\n", blast_id, (unsigned)packets_in_this_blast);
                }

                state = S_WAIT_REC_MISS;
                break;
            }

            case S_WAIT_REC_MISS: {
                /* Wait for REC_MISS and retransmit missing packets until empty */
                pkt_rec_miss_hdr_t miss;
                struct sockaddr_in from;
                ssize_t rn = udp_recv(sockfd, &miss, sizeof(miss), &from);

                if (rn <= 0) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        /* Timeout → retransmit IS_BLAST_OVER */
                        printf("[Sender] Timeout waiting for REC_MISS → retransmitting IS_BLAST_OVER...\n");

                        pkt_is_blast_over_t iso;
                        build_pkt_is_blast_over(&iso, blast_id, last_packets_in_blast);
                        udp_send(sockfd, &iso, sizeof(iso), &receiver_addr);

                        continue;   // stay in S_WAIT_REC_MISS
                    }

                    continue;  // some other recv error → ignore (rare)
                }

                if (miss.type != PKT_REC_MISS_HDR) {
                    /* ignore unrelated packets */
                    continue;
                }

                printf("[Sender] Got REC_MISS for blast=%u missing=%u\n", blast_id, (unsigned)miss.n_packets_missing);

                if (miss.n_packets_missing == 0) {
                    /* Blast complete. If this was the last blast, go to LAST_BLAST_DONE else next blast. */
                    if (blast_id >= total_blasts) {
                        state = S_LAST_BLAST_DONE;
                    } else {
                        blast_id++;
                        state = S_SENDING_BLASTS;
                    }
                    break;
                }

                /* Retransmit missing packets */
                for (uint32_t i = 0; i < max_packets_per_blast; ++i) {
                    if (i >= DEFAULT_PACKETS_PER_BLAST) break; /* safety */
                    if (miss.is_pkt_missing[i]) {
                        uint32_t pkt_idx = i;
                        uint32_t rec_start = last_blast_start_index + pkt_idx * max_records_per_packet;

                        printf("[Sender] Retransmitting blast %u packet %u (rec_start=%u)\n",
                               blast_id, pkt_idx, rec_start + 1);

                        ssize_t sent = send_blast_packet(sockfd, &receiver_addr,
                                                         blast_id, pkt_idx,
                                                         last_packets_in_blast,
                                                         records, rec_start, nrec,
                                                         max_records_per_packet);
                        if (sent < 0) {
                            fprintf(stderr, "[Sender] Failed to retransmit packet %u for blast %u\n",
                                    (unsigned)pkt_idx, (unsigned)blast_id);
                        }
                    }
                }

                /* After retransmits, send IS_BLAST_OVER again to get fresh REC_MISS */
                pkt_is_blast_over_t iso;
                build_pkt_is_blast_over(&iso, blast_id, last_packets_in_blast);
                udp_send(sockfd, &iso, sizeof(iso), &receiver_addr);
                printf("[Sender] Sent IS_BLAST_OVER (retransmit round) for blast=%u\n", blast_id);
                /* remain in S_WAIT_REC_MISS to wait for next REC_MISS */
                break;
            }

            case S_LAST_BLAST_DONE: {
                /* Send DISCONNECT, then finish. */
                printf("[Sender] All blasts done. Sending DISCONNECT...\n");
                pkt_disconnect_t disc;
                build_pkt_disconnect(&disc, 0);
                udp_send(sockfd, &disc, sizeof(disc), &receiver_addr);
                state = S_DONE;
                break;
            }

            default:
                state = S_DONE;
                break;
        } /* switch */
    } /* while */

    /* cleanup */
    close(sockfd);
    free_records(records, nrec);

    printf("[Sender] Done.\n");

    net_stats_t st = net_get_stats();
    printf("\n[NET-STATS]\n");
    printf(" Sent packets:     %lu\n", st.sent_pkts);
    printf(" Received packets: %lu\n", st.recv_pkts);
    printf(" Dropped outgoing: %lu\n", st.dropped_outgoing);
    printf(" Dropped incoming: %lu\n", st.dropped_incoming);

    return 0;
}

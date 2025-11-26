// src/receiver.c — Option A receiver implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../include/protocol.h"
#include "../include/net.h"
#include "../include/blast.h"
#include "../include/record.h"
#include "../include/fileio.h"

typedef enum {
    R_IDLE,
    R_HDR_RECV,
    R_WAIT_BLAST,
    R_RECEIVING_BLAST,
    R_SEND_REC_MISS,
    R_WAIT_DISCONNECT,
    R_DONE
} receiver_state_t;

int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: %s <port> <output_filename>\n", argv[0]);
        return 1;
    }

    uint16_t port = (uint16_t)atoi(argv[1]);

    /* ---- Prepare socket ---- */
    int sockfd = create_udp_socket();
    if (sockfd < 0) return 1;

    if (bind_udp_socket(sockfd, port) < 0) return 1;

    printf("[Receiver] Listening on port %u...\n", port);

    /* ---- State machine ---- */
    receiver_state_t state = R_IDLE;
    struct sockaddr_in sender_addr;

    /* Buffers for tracking a single blast */
    uint8_t packets_received[DEFAULT_PACKETS_PER_BLAST];
    memset(packets_received, 0, sizeof(packets_received));
    uint32_t expected_packets_this_blast = 0;
    uint32_t current_blast_id = 0;
    // uint32_t current_blast_start_rec = 0;
    // uint32_t current_blast_nrec = 0;

    /* We'll store records' data for the current blast in one blob (allocated on demand).
       Size required = expected_packets_this_blast * DEFAULT_RECORDS_PER_PACKET * rec_size
       But to simplify we allocate records_per_blast * rec_size once FILE_HDR is processed. */
    uint8_t *blast_blob = NULL;
    size_t blast_blob_size = 0;
    uint16_t rec_size = DEFAULT_RECORD_SIZE; /* will be updated from FILE_HDR */
    uint64_t bytes_written = 0;

    uint8_t recv_buf[4096];

    /* File to write to */
    FILE *outf = NULL;
    char out_filename[MAX_FILENAME_LEN + 32];
    uint64_t total_file_size = 0;
    uint32_t total_records_expected = 0;

    while (state != R_DONE) {
        switch (state) {

            case R_IDLE: {
                printf("[Receiver] Waiting for FILE_HDR...\n");
                ssize_t n = udp_recv(sockfd, recv_buf, sizeof(recv_buf), &sender_addr);
                if (n <= 0) continue;

                pkt_file_hdr_t *hdr = (pkt_file_hdr_t*)recv_buf;
                if (hdr->type != PKT_FILE_HDR) continue;

                /* copy filename & open file */
                snprintf(out_filename, sizeof(out_filename), "data/output/%.*s", MAX_FILENAME_LEN, argv[2]);
                outf = fopen(out_filename, "wb");
                if (!outf) {
                    perror("fopen output");
                    fprintf(stderr, "[Receiver] Failed to open output file: %s\n", out_filename);
                    return 1;
                }

                /* adopt negotiated values */
                if (hdr->_max_rec_size > 0) rec_size = hdr->_max_rec_size;
                total_file_size = hdr->file_size;
                total_records_expected = (uint32_t)((total_file_size + rec_size - 1) / rec_size);

                printf("[Receiver] Received FILE_HDR: filename=%s filesize=%lu records=%u rec_size=%u\n",
                       hdr->filename, total_file_size, total_records_expected, rec_size);

                /* prepare a blast blob sized for one blast = DEFAULT_PACKETS_PER_BLAST * DEFAULT_RECORDS_PER_PACKET * rec_size
                   (we'll resize later if needed based on hdr values) */
                if (blast_blob) { free(blast_blob); blast_blob = NULL; blast_blob_size = 0; }
                size_t desired = (size_t)DEFAULT_PACKETS_PER_BLAST * (size_t)DEFAULT_RECORDS_PER_PACKET * (size_t)rec_size;
                blast_blob = malloc(desired);
                if (!blast_blob) {
                    perror("malloc blast_blob");
                    fclose(outf);
                    return 1;
                }
                blast_blob_size = desired;
                memset(blast_blob, '\0', blast_blob_size);

                /* ACK the header with receiver's defaults/state */
                pkt_file_hdr_ack_t ack;
                build_pkt_file_hdr_ack(&ack, 0, DEFAULT_RECORD_SIZE,
                                       DEFAULT_RECORDS_PER_PACKET, DEFAULT_PACKETS_PER_BLAST);
                udp_send(sockfd, &ack, sizeof(ack), &sender_addr);
                printf("[Receiver] Sent FILE_HDR_ACK.\n");

                state = R_WAIT_BLAST;
                break;
            }

            case R_WAIT_BLAST: {
                printf("[Receiver] Waiting for first BLAST_PKT or IS_BLAST_OVER...\n");
                ssize_t n = udp_recv(sockfd, recv_buf, sizeof(recv_buf), &sender_addr);
                if (n <= 0) continue;

                uint8_t type = recv_buf[0];
                if (type == PKT_BLAST_PACKET) {
                    pkt_blast_packet_t *bp = (pkt_blast_packet_t*)recv_buf;

                    /* initialize blast tracking using the packet header's n_packets (sender should set it) */
                    current_blast_id = bp->blast_id;
                    expected_packets_this_blast = bp->n_packets;
                    if (expected_packets_this_blast == 0) expected_packets_this_blast = 1; /* safety */

                    memset(packets_received, 0, sizeof(packets_received));

                    /* copy records into blast_blob at proper offsets */
                    uint32_t base_packet = bp->packet_id;
                    if (base_packet < DEFAULT_PACKETS_PER_BLAST) packets_received[base_packet] = 1;

                    /* Copy each record into appropriate offset in blast_blob */
                    for (uint32_t i = 0; i < bp->n_records; ++i) {
                        // uint32_t global_rec_id = bp->record_id[i]; /* 1-based */
                        /* compute index relative to blast start for bookkeeping:
                           we don't know blast_start rec here; we'll place by (packet_id * records_per_packet + i) */
                        uint32_t rec_index_in_blast = bp->packet_id * DEFAULT_RECORDS_PER_PACKET + i;
                        size_t dest_off = (size_t)rec_index_in_blast * rec_size;
                        if (dest_off + rec_size <= blast_blob_size) {
                            memset(blast_blob + dest_off, '\0', rec_size);
                            memcpy(blast_blob + dest_off, bp->data[i], rec_size);
                        }
                        // printf("    Recv record %u (placed at offset %zu)\n", global_rec_id, dest_off);
                    }

                    state = R_RECEIVING_BLAST;
                }
                else if (type == PKT_IS_BLAST_OVER) {
                    /* no data packets arrived for this blast, still respond */
                    pkt_is_blast_over_t *iso = (pkt_is_blast_over_t*)recv_buf;
                    current_blast_id = iso->blast_id;
                    expected_packets_this_blast = iso->n_packets;
                    // if (expected_packets_this_blast == 0) expected_packets_this_blast = 1;

                    memset(packets_received, 0, sizeof(packets_received));
                    printf("[Receiver] Got IS_BLAST_OVER(blast %u) without data; replying empty or missing.\n", current_blast_id);

                    /* send REC_MISS (likely empty) */
                    pkt_rec_miss_hdr_t miss;
                    miss.type = PKT_REC_MISS_HDR;
                    miss.n_packets_missing = 0;
                    memset(miss.is_pkt_missing, 0, sizeof(miss.is_pkt_missing));
                    udp_send(sockfd, &miss, sizeof(miss), &sender_addr);
                    printf("[Receiver] ****NO BLAST PACKET SENT BUT SENT PKT_IS_BLAST_OVER****\n");
                    printf("[Receiver] Sent REC_MISS(empty)\n");

                    state = R_WAIT_DISCONNECT;
                }
                break;
            }

            case R_RECEIVING_BLAST: {
                /* keep receiving packets until IS_BLAST_OVER */
                ssize_t n = udp_recv(sockfd, recv_buf, sizeof(recv_buf), &sender_addr);
                if (n <= 0) continue;

                uint8_t type = recv_buf[0];
                if (type == PKT_BLAST_PACKET) {
                    pkt_blast_packet_t *bp = (pkt_blast_packet_t*)recv_buf;
                    printf("[Receiver] BLAST %u packet %u received (%u records, n_packets=%u)\n",
                           bp->blast_id, bp->packet_id, bp->n_records, bp->n_packets);

                    if (bp->packet_id < DEFAULT_PACKETS_PER_BLAST)
                        packets_received[bp->packet_id] = 1;

                    /* copy records */
                    for (uint32_t i = 0; i < bp->n_records; ++i) {
                        // uint32_t global_rec_id = bp->record_id[i];
                        uint32_t rec_index_in_blast = bp->packet_id * DEFAULT_RECORDS_PER_PACKET + i;
                        size_t dest_off = (size_t)rec_index_in_blast * rec_size;
                        if (dest_off + rec_size <= blast_blob_size) {
                            memset(blast_blob + dest_off, '\0', rec_size);
                            memcpy(blast_blob + dest_off, bp->data[i], rec_size);
                        }
                        // printf("    Recv record %u (placed at offset %zu)\n", global_rec_id, dest_off);
                    }
                }
                else if (type == PKT_IS_BLAST_OVER) {
                    pkt_is_blast_over_t *iso = (pkt_is_blast_over_t*)recv_buf;
                    printf("[Receiver] Received IS_BLAST_OVER for blast=%u\n", iso->blast_id);
                    current_blast_id = iso->blast_id;
                    expected_packets_this_blast = iso->n_packets;
                    state = R_SEND_REC_MISS;
                }
                break;
            }

            case R_SEND_REC_MISS: {
                /* compute missing packets using packets_received[] */
                pkt_rec_miss_hdr_t miss;
                miss.type = PKT_REC_MISS_HDR;
                uint8_t missing = 0;
                /* Only check packets that this blast is supposed to have */
                missing = 0;
                for (uint32_t i = 0; i < expected_packets_this_blast; ++i) {
                    miss.is_pkt_missing[i] = (packets_received[i] == 0);
                    if (miss.is_pkt_missing[i]) missing++;
                }

                /* Beyond the actual count, force to 0 (these packets do not exist) */
                for (uint32_t i = expected_packets_this_blast; i < DEFAULT_PACKETS_PER_BLAST; ++i) {
                    miss.is_pkt_missing[i] = 0;
                }

                miss.n_packets_missing = missing;

                udp_send(sockfd, &miss, sizeof(miss), &sender_addr);
                if (missing == 0) {
                    printf("[Receiver] Sent REC_MISS(empty)\n");
                    state = R_WAIT_DISCONNECT;
                } else {
                    printf("[Receiver] Sent REC_MISS(%u missing)\n", missing);
                    state = R_RECEIVING_BLAST; /* expect retransmitted packets */
                }
                break;
            }

            case R_WAIT_DISCONNECT: {
                /* At this point we have the blast's records in blast_blob — write them out to file.
                   We'll compute how many records to write using expected_packets_this_blast * DEFAULT_RECORDS_PER_PACKET (but clamp to total file size). */

                printf("[Receiver] Writing blast %u data to disk...\n", current_blast_id);
                /* compute how many records we actually have available to write */
                uint32_t recs_to_write = expected_packets_this_blast * DEFAULT_RECORDS_PER_PACKET;
                if (recs_to_write > total_records_expected) recs_to_write = total_records_expected;

                size_t bytes_to_write = (size_t)recs_to_write * rec_size;

                /* compute exact remaining bytes in the file */
                uint64_t remaining = total_file_size - bytes_written;
                size_t safe_write = (bytes_to_write < remaining) ? bytes_to_write : (size_t)remaining;
                if (safe_write > 0) {
                    if (fwrite(blast_blob, 1, safe_write, outf) != safe_write) {
                        perror("fwrite blast");
                    } else {
                        printf("[Receiver] Wrote %zu bytes for blast %u to %s\n",
                            safe_write, current_blast_id, out_filename);
                        bytes_written += safe_write;
                        memset(blast_blob, '\0', blast_blob_size); /* clear for next blast */
                    }
                }

                /* Now wait for either next blast packet, new IS_BLAST_OVER (rare) or DISCONNECT */
                ssize_t n = udp_recv(sockfd, recv_buf, sizeof(recv_buf), &sender_addr);
                if (n <= 0) continue;

                uint8_t type = recv_buf[0];
                if (type == PKT_BLAST_PACKET) {
                    pkt_blast_packet_t *bp = (pkt_blast_packet_t*)recv_buf;
                    memset(packets_received, 0, sizeof(packets_received));
                    current_blast_id = bp->blast_id;
                    expected_packets_this_blast = bp->n_packets;
                    if (expected_packets_this_blast == 0) expected_packets_this_blast = 1;
                    printf("[Receiver] New BLAST %u begins (packets=%u)\n", current_blast_id, expected_packets_this_blast);

                    if (bp->packet_id < DEFAULT_PACKETS_PER_BLAST) packets_received[bp->packet_id] = 1;
                    for (uint32_t i = 0; i < bp->n_records; ++i) {
                        uint32_t rec_index_in_blast = bp->packet_id * DEFAULT_RECORDS_PER_PACKET + i;
                        size_t dest_off = (size_t)rec_index_in_blast * rec_size;
                        if (dest_off + rec_size <= blast_blob_size) {
                            memset(blast_blob + dest_off, '\0', rec_size);
                            memcpy(blast_blob + dest_off, bp->data[i], rec_size);
                        }
                        // printf("    Recv record %u (placed at offset %zu)\n", bp->record_id[i], dest_off);
                    }

                    state = R_RECEIVING_BLAST;
                }
                else if (type == PKT_IS_BLAST_OVER) {
                    pkt_is_blast_over_t *iso = (pkt_is_blast_over_t*)recv_buf;
                    printf("[Receiver] ****Got IS_BLAST_OVER for new blast=%u (no packets)****\n", iso->blast_id);
                    expected_packets_this_blast = iso->n_packets;
                    pkt_rec_miss_hdr_t miss;
                    miss.type = PKT_REC_MISS_HDR;
                    miss.n_packets_missing = 0;
                    memset(miss.is_pkt_missing, 0, sizeof(miss.is_pkt_missing));
                    udp_send(sockfd, &miss, sizeof(miss), &sender_addr);
                    printf("[Receiver] Sent REC_MISS(empty)\n");
                    state = R_WAIT_DISCONNECT;
                }
                else if (type == PKT_DISCONNECT) {
                    printf("[Receiver] Received DISCONNECT — closing.\n");
                    state = R_DONE;
                }
                break;
            }

            default:
                state = R_DONE;
                break;
        } /* switch */
    } /* while */

    /* cleanup */
    if (outf) fclose(outf);
    if (blast_blob) free(blast_blob);
    close(sockfd);
    printf("[Receiver] Done.\n");
    return 0;
}

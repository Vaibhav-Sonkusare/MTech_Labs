// src/sender.c
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/blast.h"
#include "../include/fileio.h"
#include "../include/net.h"
#include "../include/protocol.h"
#include "../include/record.h"
#define MAX_FILENAME_LEN 255
#define MAX_RETRIES 5
#define RETRY_TIMEOUT_SEC 1

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
static ssize_t
send_blast_packet(int sockfd, const struct sockaddr_in *to, uint32_t blast_id,
                  uint32_t packet_id,
                  uint32_t n_packets, /* total packets in this blast */
                  const record_t *records, uint32_t start_idx,
                  uint32_t nrec_total, uint16_t max_records_per_packet) {
  pkt_blast_packet_t pkt;
  memset(&pkt, 0, sizeof(pkt));

  /* build packet payload (record ids + data) */
  uint16_t nput =
      build_blast_packet(&pkt, blast_id, packet_id, records, start_idx,
                         nrec_total, max_records_per_packet);
  /* set n_packets so receiver knows how many packets to expect in this blast */
  pkt.n_packets = htonl((uint32_t)n_packets);

  /* sanity */
  pkt.type = PKT_BLAST_PACKET;
  pkt.packet_id = htonl(packet_id);
  pkt.n_records = htonl(nput);

  ssize_t sent = udp_send(sockfd, &pkt, sizeof(pkt), to);
  if (sent < 0)
    perror("udp_send blast pkt");
  return sent;
}

int main(int argc, char **argv) {
  if (argc != 5) {
    printf("Usage: %s <receiver_ip> <port> <filename> <loss_rate>\n", argv[0]);
    return 1;
  }

  const char *receiver_ip = argv[1];
  uint16_t port = (uint16_t)atoi(argv[2]);
  const char *filename = argv[3];
  double loss_rate = atof(argv[4]);
  net_set_loss_rate(loss_rate);

  /* ---- Read file info ---- */
  /* No longer reading everything into RAM. Just open file. */
  uint64_t file_size = 0;
  FILE *infile = file_open_read(filename, &file_size);
  if (!infile) {
    fprintf(stderr, "Failed to open input file: %.*s\n", MAX_FILENAME_LEN,
            filename);
    return 1;
  }

  /* calculate total records based on DEFAULT_RECORD_SIZE initially (will update
   * later) */
  uint16_t rec_size = DEFAULT_RECORD_SIZE;
  uint32_t nrec = (uint32_t)((file_size + rec_size - 1) / rec_size);
  if (nrec == 0)
    nrec = 1;

  printf("[Sender] File '%.*s' size=%lu bytes -> %u records (rec_size=%u)\n",
         MAX_FILENAME_LEN, filename, file_size, nrec, rec_size);

  /* ---- Prepare socket ---- */
  int sockfd = create_udp_socket();
  if (sockfd < 0) {
    fclose(infile);
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

  uint16_t max_records_per_packet = ntohs(hdr._max_records_per_packet);
  uint16_t max_packets_per_blast = ntohs(hdr._max_packets_per_blast);
  if (max_records_per_packet == 0)
    max_records_per_packet = DEFAULT_RECORDS_PER_PACKET;
  if (max_packets_per_blast == 0)
    max_packets_per_blast = DEFAULT_PACKETS_PER_BLAST;

  uint32_t records_per_blast =
      (uint32_t)max_records_per_packet * (uint32_t)max_packets_per_blast;
  if (records_per_blast == 0) {
    fprintf(stderr, "Invalid blast sizing (zero)\n");
    close(sockfd);
    if (infile)
      fclose(infile);
    return 1;
  }

  uint32_t total_blasts = (nrec + records_per_blast - 1) / records_per_blast;

  /* ---- FSM ---- */
  sender_state_t state = S_IDLE;
  uint32_t blast_id = 1;

  /* variables used across states */
  uint32_t last_packets_in_blast = 0;

  /* Blast buffer management (static to persist across states) */
  static uint8_t *blast_buf = NULL;
  static size_t blast_buf_len = 0;
  static record_t *blast_recs = NULL;

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

      int retries = MAX_RETRIES;

      while (retries > 0) {
        ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);

        if (n > 0 && ack.type == PKT_FILE_HDR_ACK) {
          printf("[Sender] Received ACK! Status=%u\n", ack.status);

          /* Adjust parameters if needed */
          if (ack.status != 0) {
            uint16_t ack_max_rec = ntohs(ack._max_records_per_packet);
            uint16_t ack_max_pbl = ntohs(ack._max_packets_per_blast);

            if (ack_max_rec > 0)
              max_records_per_packet = ack_max_rec;
            if (ack_max_pbl > 0)
              max_packets_per_blast = ack_max_pbl;

            if (max_packets_per_blast > DEFAULT_PACKETS_PER_BLAST)
              max_packets_per_blast = DEFAULT_PACKETS_PER_BLAST;

            records_per_blast = (uint32_t)max_records_per_packet *
                                (uint32_t)max_packets_per_blast;

            total_blasts = (nrec + records_per_blast - 1) / records_per_blast;

            printf("[Sender] Adjusted max_records_per_packet=%u, "
                   "max_packets_per_blast=%u\n",
                   max_records_per_packet, max_packets_per_blast);
          }

          state = S_SENDING_BLASTS;
          break; // exit retry loop
        }

        /* Timeout or wrong packet → retransmit FILE_HDR */
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          retries--;
          printf("[Sender] No ACK (timeout). Resending FILE_HDR... (%d "
                 "retries left)\n",
                 retries);

          if (udp_send(sockfd, &hdr, sizeof(hdr), &receiver_addr) < 0) {
            perror("udp_send FILE_HDR");
            state = S_DONE;
            break;
          }
        }
        /* else if other error, ignore or break */
      }

      if (state == S_HDR_SENT) {
        /* Exhausted retries or error */
        if (retries <= 0) {
          fprintf(stderr,
                  "[Sender] ERROR: No FILE_HDR_ACK after retries. Aborting.\n");
        }
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
      uint32_t blast_start_rec_index = (blast_id - 1) * records_per_blast;
      uint32_t remain = nrec - blast_start_rec_index;
      if (remain ==
          0) { // Should not happen if blast_id > total_blasts check is correct
        state = S_LAST_BLAST_DONE;
        break;
      }

      uint32_t records_in_this_blast =
          (remain > records_per_blast) ? records_per_blast : remain;
      uint32_t packets_in_this_blast =
          (records_in_this_blast + max_records_per_packet - 1) /
          max_records_per_packet;

      last_packets_in_blast = packets_in_this_blast;

      printf("[Sender] Sending blast %u/%u (records %u-%u, packets %u)\n",
             blast_id, total_blasts, blast_start_rec_index + 1,
             blast_start_rec_index + records_in_this_blast,
             packets_in_this_blast);

      /* Allocate/reallocate blast buffer */
      size_t needed_buf = (size_t)records_in_this_blast * rec_size;
      if (blast_buf_len < needed_buf) {
        if (blast_buf)
          free(blast_buf);
        if (blast_recs)
          free(blast_recs);

        blast_buf = calloc(1, needed_buf);
        blast_recs = calloc(records_per_blast, sizeof(record_t));
        blast_buf_len = needed_buf;
      }

      /* seek and read */
      uint64_t file_offset = (uint64_t)blast_start_rec_index * rec_size;
      if (fseeko(infile, (off_t)file_offset, SEEK_SET) != 0) {
        perror("fseeko");
        state = S_DONE;
        break;
      }

      /* We want to read enough data for this blast. */
      int bytes_to_read = (int)records_in_this_blast * rec_size;

      /* Reset buffer to 0 */
      memset(blast_buf, 0, needed_buf);

      int nread = file_read_chunk(infile, blast_buf, bytes_to_read);
      if (nread < 0) {
        fprintf(stderr,
                "[Sender] Error reading chunk for blast %u. Aborting.\n",
                blast_id);
        state = S_DONE;
        break;
      }

      /* Setup the record structures for this blast */
      /* All records inside this buffer */
      int nrec_in_this_blast = (nread + rec_size - 1) / rec_size;
      if (nread == 0)
        nrec_in_this_blast = 0;

      /* Populate blast_recs */
      for (uint32_t i = 0; i < records_per_blast; ++i) {
        blast_recs[i].record_id = blast_start_rec_index + i + 1; /* global ID */
        blast_recs[i].data = blast_buf + i * rec_size;

        if (i < (uint32_t)nrec_in_this_blast) {
          /* check if it's the last part of file */
          int start_byte = i * rec_size;
          int end_byte = start_byte + rec_size;
          if (end_byte > nread) {
            blast_recs[i].size = nread - start_byte;
          } else {
            blast_recs[i].size = rec_size;
          }
        } else {
          blast_recs[i].size = 0; /* empty, shouldn't send */
        }
      }

      /* Send all packets that belong to this blast (packet_id
       * 0..packets_in_this_blast-1) */
      for (uint32_t pkt_idx = 0; pkt_idx < packets_in_this_blast; ++pkt_idx) {

        uint32_t local_idx = pkt_idx * max_records_per_packet;

        ssize_t sent = send_blast_packet(
            sockfd, &receiver_addr, blast_id, pkt_idx, packets_in_this_blast,
            blast_recs, local_idx,
            records_per_blast /* limit to this blast buffer size */,
            max_records_per_packet);
        if (sent < 0) {
          fprintf(stderr,
                  "[Sender] Failed to send blast packet %u/%u for blast %u\n",
                  (unsigned)pkt_idx, (unsigned)packets_in_this_blast,
                  (unsigned)blast_id);
        }
      }

      /* After sending the blast packets, send IS_BLAST_OVER and move to wait
       * for REC_MISS */
      pkt_is_blast_over_t iso;
      build_pkt_is_blast_over(&iso, blast_id, packets_in_this_blast);
      if (udp_send(sockfd, &iso, sizeof(iso), &receiver_addr) < 0) {
        perror("udp_send IS_BLAST_OVER");
      } else {
        printf("[Sender] Sent IS_BLAST_OVER (blast=%u, packets=%u)\n", blast_id,
               (unsigned)packets_in_this_blast);
      }

      state = S_WAIT_REC_MISS;
      break;
    }

    case S_WAIT_REC_MISS: {
      /* Wait for REC_MISS and retransmit missing packets until empty */
      static int miss_retries = 0;
      pkt_rec_miss_hdr_t miss;
      struct sockaddr_in from;
      ssize_t rn = udp_recv(sockfd, &miss, sizeof(miss), &from);

      if (rn <= 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          /* Timeout → retransmit IS_BLAST_OVER */
          miss_retries++;
          if (miss_retries > MAX_RETRIES) {
            fprintf(stderr,
                    "[Sender] Max retries reached waiting for REC_MISS (blast "
                    "%u). Aborting.\n",
                    blast_id);
            state = S_DONE;
            break;
          }

          printf("[Sender] Timeout waiting for REC_MISS → retransmitting "
                 "IS_BLAST_OVER... (%d/%d)\n",
                 miss_retries, MAX_RETRIES);

          pkt_is_blast_over_t iso;
          build_pkt_is_blast_over(&iso, blast_id, last_packets_in_blast);
          udp_send(sockfd, &iso, sizeof(iso), &receiver_addr);

          continue; // stay in S_WAIT_REC_MISS
        }
        continue; // some other recv error → ignore
      }

      miss_retries = 0;

      if (miss.type != PKT_REC_MISS_HDR) {
        /* ignore unrelated packets */
        continue;
      }

      /* The REC_MISS packet does not contain blast_id in this version of
         protoco. We assume it corresponds to the current blast we just sent
         IS_BLAST_OVER for. */

      uint32_t missing_count = miss.n_packets_missing;
      printf("[Sender] Got REC_MISS for blast=%u missing=%u\n", blast_id,
             missing_count);

      if (missing_count == 0) {
        /* No packets missing -> blast done */
        blast_id++;
        state = S_SENDING_BLASTS; /* move to next blast */
        break;
      }

      /* Otherwise, parse the missing list */
      /* The payload follows the header. */
      /* NOTE: In this simple protocol, the missing list is part of the struct
       * if it fits, or we need to handle variable size.
       * The struct pkt_rec_miss_hdr_t has is_pkt_missing array of size
       * DEFAULT_PACKETS_PER_BLAST. So we just check that array.
       */

      /* Retransmit missing packets */
      for (uint32_t i = 0; i < max_packets_per_blast; ++i) {
        if (i >= DEFAULT_PACKETS_PER_BLAST)
          break; /* safety */
        if (miss.is_pkt_missing[i]) {
          uint32_t pkt_idx = i;
          uint32_t local_idx = pkt_idx * max_records_per_packet;

          printf("[Sender] Retransmitting blast %u packet %u\n", blast_id,
                 pkt_idx);

          ssize_t sent = send_blast_packet(
              sockfd, &receiver_addr, blast_id, pkt_idx, last_packets_in_blast,
              blast_recs, local_idx, records_per_blast, max_records_per_packet);
          if (sent < 0) {
            fprintf(stderr,
                    "[Sender] Failed to retransmit packet %u for blast %u\n",
                    (unsigned)pkt_idx, (unsigned)blast_id);
          }
        }
      }
      /* Stay in S_WAIT_REC_MISS until we get clean slate */
      break;
    }

    case S_LAST_BLAST_DONE: {
      printf("[Sender] All blasts done. Sending DISCONNECT...\n");
      pkt_disconnect_t fin;
      build_pkt_disconnect(&fin, 0);

      /* Send a few times to ensure arrival */
      for (int k = 0; k < 5; ++k) {
        udp_send(sockfd, &fin, sizeof(fin), &receiver_addr);
      }
      state = S_DONE;
      break;
    }

    case S_DONE:
      break;

    default:
      break;
    } /* switch */
  } /* while */

  /* cleanup */
  if (infile)
    fclose(infile);
  close(sockfd);
  if (blast_buf)
    free(blast_buf);
  if (blast_recs)
    free(blast_recs);

  printf("[Sender] Done.\n");

  net_stats_t st = net_get_stats();
  printf("\n[NET-STATS]\n");
  printf(" Sent packets:     %lu\n", st.sent_pkts);
  printf(" Received packets: %lu\n", st.recv_pkts);
  printf(" Dropped outgoing: %lu\n", st.dropped_outgoing);
  printf(" Dropped incoming: %lu\n", st.dropped_incoming);

  return 0;
}

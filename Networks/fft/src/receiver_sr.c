/*
 * Selective Repeat Receiver
 *
 * Window-based protocol with buffering:
 * - Accept out-of-order packets and buffer them
 * - Send individual ACK for each packet
 * - Write to file when base packet arrives
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/net.h"
#include "../include/protocol.h"

#define MAX_RETRIES 500
#define PACKET_SIZE 1024
#define WINDOW_SIZE 16
#define MAX_FILENAME_LEN 255

/* Packet types for Selective Repeat */
#define SR_DATA 1
#define SR_ACK 2
#define SR_FIN 3
#define SR_FIN_ACK 4

#pragma pack(push, 1)
typedef struct {
  uint8_t type;
  uint32_t seq_num;
  uint32_t data_len;
  uint8_t data[PACKET_SIZE];
} sr_data_pkt_t;

typedef struct {
  uint8_t type;
  uint32_t ack_num;
} sr_ack_pkt_t;
#pragma pack(pop)

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("Usage: %s <port> <output_filename> <loss_rate>\n", argv[0]);
    return 1;
  }

  char *endptr;
  long port_l = strtol(argv[1], &endptr, 10);
  if (*endptr != '\0' || port_l <= 0 || port_l > 65535) {
    fprintf(stderr, "Invalid port: %s\n", argv[1]);
    return 1;
  }
  uint16_t port = (uint16_t)port_l;

  (void)argv[2];

  double loss_rate = strtod(argv[3], &endptr);
  if (*endptr != '\0' || loss_rate < 0.0 || loss_rate > 1.0) {
    fprintf(stderr, "Invalid loss rate: %s\n", argv[3]);
    return 1;
  }
  net_set_loss_rate(loss_rate);

  /* Create and bind socket */
  int sockfd = create_udp_socket();
  if (sockfd < 0)
    return 1;

  if (bind_udp_socket(sockfd, port) < 0)
    return 1;

  printf("[SR-Receiver] Listening on port %u...\n", port);

  struct sockaddr_in sender_addr;
  uint8_t recv_buf[4096];

  /* Wait for file header */
  FILE *outf = NULL;
  uint64_t total_file_size = 0;
  uint32_t total_packets = 0;
  int retries = 0;

  while (!outf && retries < MAX_RETRIES) {
    ssize_t n = udp_recv(sockfd, recv_buf, sizeof(recv_buf), &sender_addr);
    if (n <= 0) {
      retries++;
      continue;
    }

    pkt_file_hdr_t *hdr = (pkt_file_hdr_t *)recv_buf;
    if (hdr->type != PKT_FILE_HDR)
      continue;

    char *safe_basename = strrchr(hdr->filename, '/');
    if (safe_basename)
      safe_basename++;
    else
      safe_basename = hdr->filename;

    char *safe_win = strrchr(safe_basename, '\\');
    if (safe_win)
      safe_basename = safe_win + 1;

    if (*safe_basename == '\0')
      safe_basename = "output.bin";

    char out_filename[MAX_FILENAME_LEN + 32];
    snprintf(out_filename, sizeof(out_filename), "data/output/%s",
             safe_basename);

    outf = fopen(out_filename, "wb");
    if (!outf) {
      perror("fopen");
      close(sockfd);
      return 1;
    }

    total_file_size = ntohll(hdr->file_size);
    total_packets =
        (uint32_t)((total_file_size + PACKET_SIZE - 1) / PACKET_SIZE);
    if (total_packets == 0)
      total_packets = 1;

    printf("[SR-Receiver] Receiving file: %s (%lu bytes, %u packets)\n",
           out_filename, total_file_size, total_packets);

    pkt_file_hdr_ack_t ack;
    build_pkt_file_hdr_ack(&ack, 0, PACKET_SIZE, 1, 1);
    udp_send(sockfd, &ack, sizeof(ack), &sender_addr);
  }

  if (!outf) {
    fprintf(stderr, "[SR-Receiver] Failed to receive header\n");
    close(sockfd);
    return 1;
  }

  /* Allocate receive buffer */
  uint8_t *received = calloc(total_packets, 1);
  sr_data_pkt_t *buffer = calloc(total_packets, sizeof(sr_data_pkt_t));

  if (!received || !buffer) {
    perror("calloc");
    fclose(outf);
    close(sockfd);
    return 1;
  }

  /* Receive packets with buffering */
  uint32_t base = 0;
  uint64_t bytes_written = 0;
  int done = 0;

  while (!done) {
    ssize_t n = udp_recv(sockfd, recv_buf, sizeof(recv_buf), &sender_addr);
    if (n <= 0)
      continue;

    sr_data_pkt_t *pkt = (sr_data_pkt_t *)recv_buf;

    if (pkt->type == SR_FIN) {
      sr_ack_pkt_t ack;
      ack.type = SR_FIN_ACK;
      ack.ack_num = pkt->seq_num;
      udp_send(sockfd, &ack, sizeof(ack), &sender_addr);
      done = 1;
      printf("[SR-Receiver] Received FIN\n");
      break;
    }

    if (pkt->type != SR_DATA)
      continue;

    uint32_t seq = ntohl(pkt->seq_num);

    if (seq < total_packets && !received[seq]) {
      /* Buffer the packet */
      received[seq] = 1;
      memcpy(&buffer[seq], pkt, sizeof(sr_data_pkt_t));

      /* Write in-order packets to file */
      while (base < total_packets && received[base]) {
        uint32_t data_len = ntohl(buffer[base].data_len);
        size_t to_write = data_len;

        if (bytes_written + to_write > total_file_size) {
          to_write = (size_t)(total_file_size - bytes_written);
        }

        if (to_write > 0) {
          fwrite(buffer[base].data, 1, to_write, outf);
          bytes_written += to_write;
        }
        base++;
      }

      if (base % 100 == 0) {
        printf("[SR-Receiver] base=%u/%u\n", base, total_packets);
      }
    }

    /* Send individual ACK */
    sr_ack_pkt_t ack;
    ack.type = SR_ACK;
    ack.ack_num = htonl(seq);
    udp_send(sockfd, &ack, sizeof(ack), &sender_addr);
  }

  free(received);
  free(buffer);
  fclose(outf);
  close(sockfd);

  printf("[SR-Receiver] Done. Wrote %lu bytes.\n", bytes_written);

  net_stats_t st = net_get_stats();
  printf("\n[NET-STATS]\n");
  printf(" Sent packets:     %lu\n", st.sent_pkts);
  printf(" Received packets: %lu\n", st.recv_pkts);
  printf(" Dropped outgoing: %lu\n", st.dropped_outgoing);
  printf(" Dropped incoming: %lu\n", st.dropped_incoming);

  return 0;
}

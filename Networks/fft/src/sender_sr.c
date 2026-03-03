/*
 * Selective Repeat Sender
 *
 * Window-based protocol with selective retransmission:
 * - Send up to N packets in window
 * - Track individual ACKs for each packet
 * - Only retransmit specifically lost packets
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/fileio.h"
#include "../include/net.h"
#include "../include/protocol.h"

#define MAX_RETRIES 500
#define PACKET_SIZE 1024
#define WINDOW_SIZE 16

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
  uint32_t ack_num; /* Individual ACK for specific packet */
} sr_ack_pkt_t;
#pragma pack(pop)

int main(int argc, char **argv) {
  if (argc != 5) {
    printf("Usage: %s <receiver_ip> <port> <filename> <loss_rate>\n", argv[0]);
    return 1;
  }

  const char *receiver_ip = argv[1];

  char *endptr;
  long port_l = strtol(argv[2], &endptr, 10);
  if (*endptr != '\0' || port_l <= 0 || port_l > 65535) {
    fprintf(stderr, "Invalid port: %s\n", argv[2]);
    return 1;
  }
  uint16_t port = (uint16_t)port_l;

  const char *filename = argv[3];

  double loss_rate = strtod(argv[4], &endptr);
  if (*endptr != '\0' || loss_rate < 0.0 || loss_rate > 1.0) {
    fprintf(stderr, "Invalid loss rate: %s\n", argv[4]);
    return 1;
  }
  net_set_loss_rate(loss_rate);

  /* Open file */
  uint64_t file_size = 0;
  FILE *infile = file_open_read(filename, &file_size);
  if (!infile) {
    fprintf(stderr, "Failed to open file: %s\n", filename);
    return 1;
  }

  uint32_t total_packets =
      (uint32_t)((file_size + PACKET_SIZE - 1) / PACKET_SIZE);
  if (total_packets == 0)
    total_packets = 1;

  printf("[SR-Sender] File '%s' size=%lu, packets=%u, window=%d\n", filename,
         file_size, total_packets, WINDOW_SIZE);

  /* Create socket */
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

  /* Send file header */
  pkt_file_hdr_t hdr;
  build_pkt_file_hdr(&hdr, filename, (uint16_t)strlen(filename), file_size);

  int hdr_acked = 0;
  int retries = 0;
  while (!hdr_acked && retries < MAX_RETRIES) {
    udp_send(sockfd, &hdr, sizeof(hdr), &receiver_addr);

    pkt_file_hdr_ack_t ack;
    struct sockaddr_in from;
    ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);

    if (n > 0 && ack.type == PKT_FILE_HDR_ACK) {
      hdr_acked = 1;
      printf("[SR-Sender] Header ACKed\n");
    } else {
      retries++;
    }
  }

  if (!hdr_acked) {
    fprintf(stderr, "[SR-Sender] Failed to get header ACK\n");
    fclose(infile);
    close(sockfd);
    return 1;
  }

  /* Pre-read all packets */
  sr_data_pkt_t *packets = calloc(total_packets, sizeof(sr_data_pkt_t));
  uint8_t *acked = calloc(total_packets, 1); /* Track which are ACKed */

  if (!packets || !acked) {
    perror("calloc");
    fclose(infile);
    close(sockfd);
    return 1;
  }

  for (uint32_t i = 0; i < total_packets; i++) {
    uint8_t buf[PACKET_SIZE] = {0};
    size_t bytes_read = fread(buf, 1, PACKET_SIZE, infile);

    packets[i].type = SR_DATA;
    packets[i].seq_num = htonl(i);
    packets[i].data_len = htonl((uint32_t)bytes_read);
    memcpy(packets[i].data, buf, bytes_read);
  }
  fclose(infile);

  /* Selective Repeat transmission */
  uint32_t base = 0;
  struct sockaddr_in from;
  sr_ack_pkt_t ack;

  while (base < total_packets) {
    /* Send all unacked packets in window */
    uint32_t window_end = base + WINDOW_SIZE;
    if (window_end > total_packets)
      window_end = total_packets;

    for (uint32_t i = base; i < window_end; i++) {
      if (!acked[i]) {
        udp_send(sockfd, &packets[i], sizeof(sr_data_pkt_t), &receiver_addr);
      }
    }

    /* Collect ACKs */
    int got_ack = 0;
    for (int tries = 0; tries < 5; tries++) {
      ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);

      if (n > 0 && ack.type == SR_ACK) {
        uint32_t ack_num = ntohl(ack.ack_num);
        if (ack_num < total_packets) {
          acked[ack_num] = 1;
          got_ack = 1;
        }
      }
    }

    /* Slide window */
    while (base < total_packets && acked[base]) {
      base++;
    }

    if (base % 100 == 0 && got_ack) {
      printf("[SR-Sender] base=%u/%u\n", base, total_packets);
    }
  }

  /* Send FIN */
  sr_data_pkt_t fin;
  memset(&fin, 0, sizeof(fin));
  fin.type = SR_FIN;
  fin.seq_num = htonl(total_packets);

  int fin_acked = 0;
  retries = 0;
  while (!fin_acked && retries < MAX_RETRIES) {
    udp_send(sockfd, &fin, sizeof(fin), &receiver_addr);

    ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);
    if (n > 0 && ack.type == SR_FIN_ACK) {
      fin_acked = 1;
    } else {
      retries++;
    }
  }

  free(packets);
  free(acked);
  close(sockfd);

  printf("[SR-Sender] Done.\n");

  net_stats_t st = net_get_stats();
  printf("\n[NET-STATS]\n");
  printf(" Sent packets:     %lu\n", st.sent_pkts);
  printf(" Received packets: %lu\n", st.recv_pkts);
  printf(" Dropped outgoing: %lu\n", st.dropped_outgoing);
  printf(" Dropped incoming: %lu\n", st.dropped_incoming);

  return 0;
}

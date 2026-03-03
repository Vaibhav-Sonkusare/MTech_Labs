/*
 * Stop-and-Wait Sender
 *
 * Simple reliable transfer protocol:
 * - Send 1 packet
 * - Wait for ACK
 * - On timeout, retransmit
 * - On ACK, send next packet
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../include/fileio.h"
#include "../include/net.h"
#include "../include/protocol.h"

#define MAX_RETRIES 500
#define PACKET_SIZE 1024

/* Simple packet types for Stop-and-Wait */
#define SAW_DATA 1
#define SAW_ACK 2
#define SAW_FIN 3
#define SAW_FIN_ACK 4

#pragma pack(push, 1)
typedef struct {
  uint8_t type;
  uint32_t seq_num;
  uint32_t data_len;
  uint8_t data[PACKET_SIZE];
} saw_data_pkt_t;

typedef struct {
  uint8_t type;
  uint32_t ack_num;
} saw_ack_pkt_t;
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

  printf("[SAW-Sender] File '%s' size=%lu, packets=%u\n", filename, file_size,
         total_packets);

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

  /* Send file header first */
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
      printf("[SAW-Sender] Header ACKed\n");
    } else {
      retries++;
    }
  }

  if (!hdr_acked) {
    fprintf(stderr, "[SAW-Sender] Failed to get header ACK\n");
    fclose(infile);
    close(sockfd);
    return 1;
  }

  /* Send packets one by one */
  saw_data_pkt_t pkt;
  saw_ack_pkt_t ack;
  struct sockaddr_in from;
  uint8_t buf[PACKET_SIZE];

  for (uint32_t seq = 0; seq < total_packets; seq++) {
    /* Read data from file */
    memset(buf, 0, PACKET_SIZE);
    size_t bytes_read = fread(buf, 1, PACKET_SIZE, infile);

    /* Build packet */
    pkt.type = SAW_DATA;
    pkt.seq_num = htonl(seq);
    pkt.data_len = htonl((uint32_t)bytes_read);
    memcpy(pkt.data, buf, bytes_read);

    /* Send and wait for ACK */
    int acked = 0;
    retries = 0;

    while (!acked && retries < MAX_RETRIES) {
      udp_send(sockfd, &pkt, sizeof(pkt), &receiver_addr);

      ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);

      if (n > 0 && ack.type == SAW_ACK && ntohl(ack.ack_num) == seq) {
        acked = 1;
      } else {
        retries++;
      }
    }

    if (!acked) {
      fprintf(stderr, "[SAW-Sender] Failed to get ACK for packet %u\n", seq);
      fclose(infile);
      close(sockfd);
      return 1;
    }

    if ((seq + 1) % 100 == 0 || seq == total_packets - 1) {
      printf("[SAW-Sender] Sent packet %u/%u\n", seq + 1, total_packets);
    }
  }

  /* Send FIN */
  saw_data_pkt_t fin;
  fin.type = SAW_FIN;
  fin.seq_num = htonl(total_packets);
  fin.data_len = 0;

  int fin_acked = 0;
  retries = 0;
  while (!fin_acked && retries < MAX_RETRIES) {
    udp_send(sockfd, &fin, sizeof(fin), &receiver_addr);

    ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);
    if (n > 0 && ack.type == SAW_FIN_ACK) {
      fin_acked = 1;
    } else {
      retries++;
    }
  }

  fclose(infile);
  close(sockfd);

  printf("[SAW-Sender] Done.\n");

  net_stats_t st = net_get_stats();
  printf("\n[NET-STATS]\n");
  printf(" Sent packets:     %lu\n", st.sent_pkts);
  printf(" Received packets: %lu\n", st.recv_pkts);
  printf(" Dropped outgoing: %lu\n", st.dropped_outgoing);
  printf(" Dropped incoming: %lu\n", st.dropped_incoming);

  return 0;
}

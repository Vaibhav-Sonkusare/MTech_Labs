/*
 * Go-Back-N Sender
 *
 * Window-based protocol:
 * - Send up to N packets in window
 * - On timeout or NACK, go back and resend from lost packet
 * - On cumulative ACK, slide window
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

/* Packet types for Go-Back-N */
#define GBN_DATA 1
#define GBN_ACK 2
#define GBN_FIN 3
#define GBN_FIN_ACK 4

#pragma pack(push, 1)
typedef struct {
  uint8_t type;
  uint32_t seq_num;
  uint32_t data_len;
  uint8_t data[PACKET_SIZE];
} gbn_data_pkt_t;

typedef struct {
  uint8_t type;
  uint32_t ack_num; /* Cumulative ACK: all packets up to this are received */
} gbn_ack_pkt_t;
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

  printf("[GBN-Sender] File '%s' size=%lu, packets=%u, window=%d\n", filename,
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
      printf("[GBN-Sender] Header ACKed\n");
    } else {
      retries++;
    }
  }

  if (!hdr_acked) {
    fprintf(stderr, "[GBN-Sender] Failed to get header ACK\n");
    fclose(infile);
    close(sockfd);
    return 1;
  }

  /* Pre-read all packets into buffer */
  gbn_data_pkt_t *packets = calloc(total_packets, sizeof(gbn_data_pkt_t));
  if (!packets) {
    perror("calloc");
    fclose(infile);
    close(sockfd);
    return 1;
  }

  for (uint32_t i = 0; i < total_packets; i++) {
    uint8_t buf[PACKET_SIZE] = {0};
    size_t bytes_read = fread(buf, 1, PACKET_SIZE, infile);

    packets[i].type = GBN_DATA;
    packets[i].seq_num = htonl(i);
    packets[i].data_len = htonl((uint32_t)bytes_read);
    memcpy(packets[i].data, buf, bytes_read);
  }
  fclose(infile);

  /* Go-Back-N transmission */
  uint32_t base = 0;     /* First unacknowledged */
  uint32_t next_seq = 0; /* Next to send */
  struct sockaddr_in from;
  gbn_ack_pkt_t ack;

  while (base < total_packets) {
    /* Send all packets in window */
    while (next_seq < base + WINDOW_SIZE && next_seq < total_packets) {
      udp_send(sockfd, &packets[next_seq], sizeof(gbn_data_pkt_t),
               &receiver_addr);
      next_seq++;
    }

    /* Wait for ACK */
    ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);

    if (n > 0 && ack.type == GBN_ACK) {
      uint32_t ack_num = ntohl(ack.ack_num);

      if (ack_num >= base) {
        base = ack_num + 1; /* Slide window */

        if (base % 100 == 0 || base == total_packets) {
          printf("[GBN-Sender] ACK %u, base=%u/%u\n", ack_num, base,
                 total_packets);
        }
      }
    } else if (n <= 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
      /* Timeout - go back to base */
      next_seq = base;
    }
  }

  /* Send FIN */
  gbn_data_pkt_t fin;
  memset(&fin, 0, sizeof(fin));
  fin.type = GBN_FIN;
  fin.seq_num = htonl(total_packets);

  int fin_acked = 0;
  retries = 0;
  while (!fin_acked && retries < MAX_RETRIES) {
    udp_send(sockfd, &fin, sizeof(fin), &receiver_addr);

    ssize_t n = udp_recv(sockfd, &ack, sizeof(ack), &from);
    if (n > 0 && ack.type == GBN_FIN_ACK) {
      fin_acked = 1;
    } else {
      retries++;
    }
  }

  free(packets);
  close(sockfd);

  printf("[GBN-Sender] Done.\n");

  net_stats_t st = net_get_stats();
  printf("\n[NET-STATS]\n");
  printf(" Sent packets:     %lu\n", st.sent_pkts);
  printf(" Received packets: %lu\n", st.recv_pkts);
  printf(" Dropped outgoing: %lu\n", st.dropped_outgoing);
  printf(" Dropped incoming: %lu\n", st.dropped_incoming);

  return 0;
}

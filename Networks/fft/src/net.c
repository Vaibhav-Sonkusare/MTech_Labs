// src/net.c

#include "net.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

/* ---------------- Global statistics ---------------- */
static net_stats_t g_stats = {0};

/* Global loss rate: 0.0 = no loss, 1.0 = all packets lost */
static double g_loss_rate = 0.0;

/* Called by sender/receiver at startup */
void net_set_loss_rate(double rate)
{
    if (rate < 0.0) rate = 0.0;
    if (rate > 1.0) rate = 1.0;
    g_loss_rate = rate;

    /* Initialize RNG once */
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }

    fprintf(stderr, "[NET] Packet loss rate set to %.3f\n", g_loss_rate);
}

/* -------------------------------
 * create_udp_socket()
 * ------------------------------- */
int create_udp_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return -1;
    }

    /* Set default timeout USING THE MACRO */
    struct timeval tv;
    tv.tv_sec  = NET_RECV_TIMEOUT_SEC;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        // Not fatal
    }

    return sockfd;
}

/* -------------------------------
 * bind_udp_socket()
 * ------------------------------- */
int bind_udp_socket(int sockfd, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return -1;
    }
    return 0;
}

/* -------------------------------
 * connect_udp_socket()
 * ------------------------------- */
int connect_udp_socket(int sockfd, const char* ip, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        return -1;
    }
    return 0;
}

/* Returns true if this packet should be dropped */
static int maybe_drop() {
    if (g_loss_rate <= 0.0) return 0;
    double r = (double)rand() / RAND_MAX;
    return (r < g_loss_rate);
}

/* -------------------- LOSSY SEND --------------------- */
ssize_t udp_send(int sockfd, const void* buf, size_t len,
                 const struct sockaddr_in* addr)
{
    g_stats.sent_pkts++;

    uint8_t pkt_type = *(uint8_t*)buf;

    if (maybe_drop()) {
        g_stats.dropped_outgoing++;
        fprintf(stderr, "[NET] DROPPED outgoing packet type=%u\n", pkt_type);
        return len; // pretend success
    }

    return sendto(sockfd, buf, len, 0,
                  (struct sockaddr*)addr, sizeof(*addr));
}


/* -------------------- LOSSY RECV --------------------- */
ssize_t udp_recv(int sockfd, void* buf, size_t maxlen,
                 struct sockaddr_in* from)
{
    socklen_t addr_len = sizeof(*from);
    ssize_t n = recvfrom(sockfd, buf, maxlen, 0,
                         (struct sockaddr*)from, &addr_len);

    if (n <= 0) return n;

    g_stats.recv_pkts++;

    uint8_t pkt_type = *(uint8_t*)buf;

    if (maybe_drop()) {
        g_stats.dropped_incoming++;
        fprintf(stderr, "[NET] DROPPED incoming packet type=%u\n", pkt_type);
        return -1;     // simulate loss
    }

    return n;
}

net_stats_t net_get_stats() {
    return g_stats; // return a COPY
}

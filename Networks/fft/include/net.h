// include/net.h

#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>

#ifndef NET_RECV_TIMEOUT_SEC
#define NET_RECV_TIMEOUT_SEC 2
#endif

/* ---- Statistics structure ---- */
typedef struct {
    uint64_t sent_pkts;
    uint64_t recv_pkts;
    uint64_t dropped_outgoing;
    uint64_t dropped_incoming;
} net_stats_t;

int create_udp_socket();
int bind_udp_socket(int sockfd, uint16_t port);
int connect_udp_socket(int sockfd, const char* ip, uint16_t port);

void net_set_loss_rate(double rate);

/* Retrieve stats (read-only copy) */
net_stats_t net_get_stats();

ssize_t udp_send(int sockfd, const void* buf, size_t len,
                 const struct sockaddr_in* addr);

ssize_t udp_recv(int sockfd, void* buf, size_t maxlen,
                 struct sockaddr_in* from);

#endif

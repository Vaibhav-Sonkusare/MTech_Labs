// net.h

#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <netinet/in.h>

int create_udp_socket();
int bind_udp_socket(int sockfd, uint16_t port);
int connect_udp_socket(int sockfd, const char* ip, uint16_t port);

ssize_t udp_send(int sockfd, const void* buf, size_t len,
                 const struct sockaddr_in* addr);

ssize_t udp_recv(int sockfd, void* buf, size_t maxlen,
                 struct sockaddr_in* from);

#endif

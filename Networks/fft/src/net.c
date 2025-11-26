// net.c

#include "net.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int create_udp_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
    }
    return sockfd;
}

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

ssize_t udp_send(int sockfd, const void* buf, size_t len,
                 const struct sockaddr_in* addr)
{
    return sendto(sockfd, buf, len, 0,
                  (struct sockaddr*)addr, sizeof(*addr));
}

ssize_t udp_recv(int sockfd, void* buf, size_t maxlen,
                 struct sockaddr_in* from)
{
    socklen_t addr_len = sizeof(*from);
    return recvfrom(sockfd, buf, maxlen, 0,
                    (struct sockaddr*)from, &addr_len);
}

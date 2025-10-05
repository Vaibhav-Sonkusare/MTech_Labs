/*
 * portscanner_fixed.c
 * Simple TCP port scanner using non-blocking connect() + select() for timeout.
 *
 * Usage:
 *   ./portscanner_fixed <host> [start_port] [end_port] [timeout_ms]
 *
 * Notes:
 *  - Scans TCP ports only (1..65535).
 *  - Works with IPv4 and IPv6.
 *  - Uses a per-port timeout (milliseconds).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#define MIN_PORT 1
#define MAX_PORT 65535

static int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

static int set_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
}

/* Returns 1 if open, 0 if closed, -1 on other error */
int scan_port(struct addrinfo *ai, int port, int timeout_ms) {
    if (port < MIN_PORT || port > MAX_PORT) return -1;

    int sockfd = socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) return -1;

    if (set_nonblocking(sockfd) < 0) {
        close(sockfd);
        return -1;
    }

    int result = 0;
    /* Prepare an address copy and set the port safely */
    if (ai->ai_family == AF_INET) {
        struct sockaddr_in addr4;
        /* use ai_addrlen or sizeof(sockaddr_in) whichever is smaller/available */
        memset(&addr4, 0, sizeof(addr4));
        memcpy(&addr4, ai->ai_addr, (ai->ai_addrlen < sizeof(addr4)) ? ai->ai_addrlen : sizeof(addr4));
        addr4.sin_port = htons((uint16_t)port);

        int err = connect(sockfd, (struct sockaddr*)&addr4, sizeof(addr4));
        if (err == 0) {
            result = 1;
            set_blocking(sockfd);
            close(sockfd);
            return result;
        } else if (errno != EINPROGRESS) {
            close(sockfd);
            return 0;
        }

    } else if (ai->ai_family == AF_INET6) {
        struct sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        memcpy(&addr6, ai->ai_addr, (ai->ai_addrlen < sizeof(addr6)) ? ai->ai_addrlen : sizeof(addr6));
        addr6.sin6_port = htons((uint16_t)port);

        int err = connect(sockfd, (struct sockaddr*)&addr6, sizeof(addr6));
        if (err == 0) {
            result = 1;
            set_blocking(sockfd);
            close(sockfd);
            return result;
        } else if (errno != EINPROGRESS) {
            close(sockfd);
            return 0;
        }
    } else {
        close(sockfd);
        return -1;
    }

    /* Wait for socket to become writable or timeout */
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int sel = select(sockfd + 1, NULL, &wfds, NULL, &tv);
    if (sel > 0) {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
            result = 0;
        } else {
            result = (so_error == 0) ? 1 : 0;
        }
    } else {
        /* select == 0 => timeout, select < 0 => error (treat as closed) */
        result = 0;
    }

    set_blocking(sockfd);
    close(sockfd);
    return result;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <host> [start_port] [end_port] [timeout_ms]\n", argv[0]);
        fprintf(stderr, "Default: start_port=1 end_port=1026 timeout_ms=500\n");
        return 1;
    }

    const char *host = argv[1];
    long start_port = MIN_PORT;
    long end_port = 1026;
    int timeout_ms = 500;

    if (argc >= 3) start_port = atol(argv[2]);
    if (argc >= 4) end_port = atol(argv[3]);
    if (argc >= 5) timeout_ms = atoi(argv[4]);

    /* Validate and clamp ports to the valid 1..65535 range */
    if (start_port < MIN_PORT) start_port = MIN_PORT;
    if (start_port > MAX_PORT) {
        fprintf(stderr, "start_port too large; clamped to %d\n", MAX_PORT);
        start_port = MAX_PORT;
    }
    if (end_port < start_port) end_port = start_port;
    if (end_port > MAX_PORT) {
        fprintf(stderr, "end_port too large; clamped to %d\n", MAX_PORT);
        end_port = MAX_PORT;
    }
    if (timeout_ms <= 0) timeout_ms = 500;

    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    int gai = getaddrinfo(host, NULL, &hints, &servinfo);
    if (gai != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return 2;
    }

    printf("Scanning %s ports %ld-%ld (timeout %d ms)\n", host, start_port, end_port, timeout_ms);
    int open_found = 0;
    for (long port = start_port; port <= end_port; ++port) {
        int is_open = 0;
        for (p = servinfo; p != NULL; p = p->ai_next) {
            int r = scan_port(p, (int)port, timeout_ms);
            if (r == 1) {
                is_open = 1;
                break;
            } else if (r == -1) {
                /* error with this family; try next one */
                continue;
            }
        }
        if (is_open) {
            printf("Port %ld: OPEN\n", port);
            ++open_found;
        }
    }

    if (!open_found) {
        printf("No open ports found in the specified range.\n");
    } else {
        printf("Scan complete. %d open port(s) found.\n", open_found);
    }

    freeaddrinfo(servinfo);
    return 0;
}


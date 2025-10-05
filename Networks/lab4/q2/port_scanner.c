/*
 * portscanner.c
 * Simple TCP port scanner using non-blocking connect() + select() for timeout.
 *
 * Usage:
 *   ./portscanner <host> [start_port] [end_port] [timeout_ms]
 *
 * Example:
 *   ./portscanner localhost 1 1026 500
 *
 * Notes:
 *  - Scans TCP ports only.
 *  - Works with IPv4 and IPv6.
 *  - Uses a per-port timeout (in milliseconds).
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
int scan_port(struct addrinfo *ai, unsigned short port, int timeout_ms) {
    int sockfd = -1;
    int result = 0;
    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", (unsigned)port);

    /* For connecting we need an addrinfo with the chosen port.
       Use getaddrinfo with ai's family/flags/protocol but port changed. */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ai->ai_family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *res = NULL;
    if (getaddrinfo(((struct sockaddr_storage*)ai->ai_addr)->ss_family == AF_INET6 ? NULL : NULL, portstr, &hints, &res) != 0) {
        /* We'll instead duplicate ai and change the port manually below. */
        res = NULL;
    }

    /* More reliable approach: copy ai->ai_addr and set port in-place */
    int sock = socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        return -1;
    }
    sockfd = sock;

    /* create a copy of sockaddr with port field set */
    if (ai->ai_family == AF_INET) {
        struct sockaddr_in addr4;
        memcpy(&addr4, ai->ai_addr, sizeof(addr4));
        addr4.sin_port = htons(port);
        if (set_nonblocking(sockfd) < 0) {
            close(sockfd);
            return -1;
        }

        int err = connect(sockfd, (struct sockaddr*)&addr4, sizeof(addr4));
        if (err == 0) {
            /* immediate connect success */
            result = 1;
            set_blocking(sockfd);
            close(sockfd);
            return result;
        } else if (errno != EINPROGRESS) {
            /* connection failed immediately */
            close(sockfd);
            return 0;
        }

        /* wait with select() for writability or error */
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
            /* timeout or select error */
            result = 0;
        }

        set_blocking(sockfd);
        close(sockfd);
        return result;
    } else if (ai->ai_family == AF_INET6) {
        struct sockaddr_in6 addr6;
        memcpy(&addr6, ai->ai_addr, sizeof(addr6));
        addr6.sin6_port = htons(port);

        if (set_nonblocking(sockfd) < 0) {
            close(sockfd);
            return -1;
        }

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
            result = 0;
        }

        set_blocking(sockfd);
        close(sockfd);
        return result;
    } else {
        close(sockfd);
        return -1;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <host> [start_port] [end_port] [timeout_ms]\n", argv[0]);
        fprintf(stderr, "Default: start_port=1 end_port=1026 timeout_ms=500\n");
        return 1;
    }

    const char *host = argv[1];
    int start_port = 1;
    int end_port = 1026;
    int timeout_ms = 500;

    if (argc >= 3) start_port = atoi(argv[2]);
    if (argc >= 4) end_port = atoi(argv[3]);
    if (argc >= 5) timeout_ms = atoi(argv[4]);

    if (start_port < 1) start_port = 1;
    if (end_port < start_port) end_port = start_port;
    if (timeout_ms <= 0) timeout_ms = 500;

    struct addrinfo hints, *servinfo, *p;
    char portstr[6];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    /* We won't pass a port here; we'll adjust sockaddr port manually in scan_port */
    int gai = getaddrinfo(host, NULL, &hints, &servinfo);
    if (gai != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
        return 2;
    }

    printf("Scanning %s ports %d-%d (timeout %d ms)\n", host, start_port, end_port, timeout_ms);
    int open_found = 0;
    for (int port = start_port; port <= end_port; ++port) {
        /* try each returned addrinfo candidate until one indicates open */
        int is_open = 0;
        for (p = servinfo; p != NULL; p = p->ai_next) {
            int r = scan_port(p, (unsigned short)port, timeout_ms);
            if (r == 1) {
                is_open = 1;
                break;
            } else if (r == -1) {
                /* problem creating socket for this family; try next family */
                continue;
            } else {
                /* closed on this family; try next family */
                continue;
            }
        }
        if (is_open) {
            printf("Port %d: OPEN\n", port);
            ++open_found;
        } else {
            /* optionally print closed ports - commented out to reduce noise */
            /* printf("Port %d: closed\n", port); */
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


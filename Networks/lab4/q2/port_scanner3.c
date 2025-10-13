/*
 * portscanner_connect.c
 *
 * Scans TCP ports (1..65535). For every open port found it opens a client
 * connection and attempts to read a short banner (up to 1024 bytes).
 *
 * Usage:
 *   ./portscanner_connect <host> [start_port] [end_port] [timeout_ms]
 *
 * Example:
 *   ./portscanner_connect localhost 1 1026 500
 *
 * Notes:
 *  - Uses non-blocking connect() + select() for per-connection timeout.
 *  - After successful connect we set a receive timeout and try to read a banner.
 *  - Works with IPv4 & IPv6.
 *  - Make sure you have permission to scan the target host.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MIN_PORT 1
#define MAX_PORT 65535
#define MAX_BANNER 1024

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

/* Perform non-blocking connect with timeout using a prepared sockaddr (already with port set).
 * Returns:
 *   1 -> connected
 *   0 -> timed out or connection refused
 *  -1 -> other error
 */
static int nonblocking_connect_with_addr(int family, const struct sockaddr *addr, socklen_t addrlen, int timeout_ms, int *out_sockfd) {
    int sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return -1;

    if (set_nonblocking(sock) < 0) {
        close(sock);
        return -1;
    }

    int r = connect(sock, addr, addrlen);
    if (r == 0) {
        /* immediate success */
        set_blocking(sock);
        *out_sockfd = sock;
        return 1;
    } else if (errno != EINPROGRESS) {
        /* immediate failure (refused, unreachable, etc.) */
        close(sock);
        return 0;
    }

    /* wait for writability */
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int sel = select(sock + 1, NULL, &wfds, NULL, &tv);
    if (sel > 0) {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
            close(sock);
            return 0;
        }
        if (so_error != 0) {
            /* connect failed */
            close(sock);
            return 0;
        }
        /* connected */
        set_blocking(sock);
        *out_sockfd = sock;
        return 1;
    } else if (sel == 0) {
        /* timeout */
        close(sock);
        return 0;
    } else {
        /* select error */
        close(sock);
        return -1;
    }
}

/* Try to read up to MAX_BANNER bytes from connected socket with recv timeout (ms).
 * Returns number of bytes read (>=0) or -1 on error.
 * Caller should close sockfd after this.
 */
static ssize_t grab_banner(int sockfd, int recv_timeout_ms, char *buf, size_t bufsz) {
    /* Set recv timeout via SO_RCVTIMEO */
    struct timeval tv;
    tv.tv_sec = recv_timeout_ms / 1000;
    tv.tv_usec = (recv_timeout_ms % 1000) * 1000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        /* non-fatal: continue but recv will block if kernel ignores it */
    }

    ssize_t n = recv(sockfd, buf, bufsz, 0);
    fprintf(stderr, "buffer:%s^^^\n", buf);
    if (n < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
            return 0;
        }
        return -1;
    }
    return n;
}

/* Helper: copy ai->ai_addr into local sockaddr and set the port value safely.
 * Returns 0 on success, -1 on unsupported family.
 * The caller supplies port (validated 1..65535).
 */
static int make_addr_with_port(struct addrinfo *ai, int port, struct sockaddr_storage *out_addr, socklen_t *out_len) {
    if (ai->ai_family == AF_INET) {
        struct sockaddr_in addr4;
        memset(&addr4, 0, sizeof(addr4));
        /* copy how much is available but not more than struct size */
        memcpy(&addr4, ai->ai_addr, (ai->ai_addrlen < sizeof(addr4)) ? ai->ai_addrlen : sizeof(addr4));
        addr4.sin_port = htons((uint16_t)port);
        memcpy(out_addr, &addr4, sizeof(addr4));
        *out_len = sizeof(addr4);
        return 0;
    } else if (ai->ai_family == AF_INET6) {
        struct sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        memcpy(&addr6, ai->ai_addr, (ai->ai_addrlen < sizeof(addr6)) ? ai->ai_addrlen : sizeof(addr6));
        addr6.sin6_port = htons((uint16_t)port);
        memcpy(out_addr, &addr6, sizeof(addr6));
        *out_len = sizeof(addr6);
        return 0;
    }
    return -1;
}

/* Check if port is open by trying nonblocking connect. We reuse the same function as the client connect,
 * but return status without keeping the socket open.
 * Returns:
 *   1 -> open (connected)
 *   0 -> closed/timeout
 *  -1 -> error
 */
static int is_port_open_for_ai(struct addrinfo *ai, int port, int timeout_ms) {
    struct sockaddr_storage addr;
    socklen_t addrlen;
    if (make_addr_with_port(ai, port, &addr, &addrlen) != 0) return -1;

    int sockfd = -1;
    int rc = nonblocking_connect_with_addr(ai->ai_family, (struct sockaddr*)&addr, addrlen, timeout_ms, &sockfd);
    if (rc == 1) {
        /* We connected — close immediately (we only wanted to test). */
        close(sockfd);
        return 1;
    }
    return rc;
}

/* After the scanner finds a port open it will call this to establish a client connection,
 * attempt to read a banner, and print results. */
static void connect_and_report(struct addrinfo *ai, int port, int timeout_ms) {
    struct sockaddr_storage addr;
    socklen_t addrlen;
    if (make_addr_with_port(ai, port, &addr, &addrlen) != 0) {
        printf("Port %d: (unsupported address family)\n", port);
        return;
    }

    int sockfd = -1;
    int rc = nonblocking_connect_with_addr(ai->ai_family, (struct sockaddr*)&addr, addrlen, timeout_ms, &sockfd);
    if (rc == 1) {
        /* connected */
        printf("Port %d: OPEN (connected)\n", port);

        /* try to read small banner (use a shorter timeout for banner) */
        char buf[MAX_BANNER+1];
        ssize_t n = grab_banner(sockfd, (timeout_ms < 20000) ? timeout_ms : 20000, buf, MAX_BANNER);
        if (n > 0) {
            /* make printable -- replace non-printable with '.' */
            size_t toshow = (size_t)n;
            // if (toshow > 512) toshow = 512; /* avoid printing huge banners */
            buf[toshow] = '\0';
            /* sanitize display */
            for (size_t i = 0; i < toshow; ++i) {
                if (!isprint((unsigned char)buf[i])) buf[i] = '.';
            }
            printf("  Data (%zd bytes): %s\n", n, buf);
        } else if (n == 0) {
            printf("  No data received (timeout or no initial data).\n");
        } else {
            printf("  Data read error: %s\n", strerror(errno));
        }

        close(sockfd);
    } else if (rc == 0) {
        /* not open (timeout/refused) */
        /* Shouldn't normally reach here because caller already detected open, but safe-check */
        printf("Port %d: closed or timed out (while trying to connect for data)\n", port);
    } else {
        printf("Port %d: error while connecting: %s\n", port, strerror(errno));
    }
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

    int open_count = 0;
    for (long port = start_port; port <= end_port; ++port) {
        int found_open = 0;
        for (p = servinfo; p != NULL; p = p->ai_next) {
            int r = is_port_open_for_ai(p, (int)port, timeout_ms);
            if (r == 1) {
                found_open = 1;
                break;
            } else if (r == -1) {
                /* error for this family: try next family */
                continue;
            }
        }
        if (found_open) {
            ++open_count;
            /* Connect a client and attempt banner grab (use first address family that worked) */
            connect_and_report(p, (int)port, timeout_ms);
        }
    }

    if (open_count == 0) {
        printf("No open ports found in the specified range.\n");
    } else {
        printf("Scan complete. %d open port(s) found.\n", open_count);
    }

    freeaddrinfo(servinfo);
    return 0;
}

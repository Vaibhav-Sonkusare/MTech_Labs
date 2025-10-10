// error_client.c  -- flip only inside the MSG bit-field
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[MAX];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // connect to localhost
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }
    while (1) {
        memset(buffer, 0, MAX);
        ssize_t n = read(sock, buffer, MAX - 1);
        if (n <= 0) {
            perror("read");
            close(sock);
            return 1;
        }
        buffer[n] = '\0';
        printf("Error Client got: %s\n", buffer);

        // find the MSG: field and the contiguous bitstring after it
        char *p = strstr(buffer, "MSG:");
        if (!p) {
            printf("No MSG: field found — forwarding unmodified.\n");
            write(sock, buffer, strlen(buffer));
            close(sock);
            return 0;
        }

        char *msg = p + 4; // start of bitstring (immediately after "MSG:")
        // compute the length of contiguous '0'/'1' characters
        int msg_len = 0;
        while (msg[msg_len] == '0' || msg[msg_len] == '1') msg_len++;

        if (msg_len == 0) {
            printf("MSG: field empty — forwarding unmodified.\n");
            write(sock, buffer, strlen(buffer));
            close(sock);
            return 0;
        }

        // seed randomness more safely
        srand((unsigned int)(time(NULL) ^ getpid()));

        int induce = rand() % 2; // 50% chance
        if (induce) {
            int bit = rand() % msg_len; // choose index inside message only
            char before = msg[bit];
            msg[bit] = (msg[bit] == '0') ? '1' : '0';
            printf("Error Client induced error at message bit index %d (0-based) — flipped %c -> %c\n",
                bit, before, msg[bit]);
        } else {
            printf("Error Client forwarded without error\n");
        }

        // print original/modified message and the whole packet for clarity
        printf("Error Client sending packet: %s\n", buffer);
        write(sock, buffer, strlen(buffer));

        if (!induce) {
            break;
        }
    }    
    close(sock);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 2048
#define HEADER_SIZE 32

// Compute number of parity bits required
int calcParityBits(int m) {
    int r = 0;
    while ((1 << r) < (m + r + 1)) r++;
    return r;
}

// Encode arbitrary length binary message using Hamming code
void hammingEncode(char *data, char *encoded) {
    int m = strlen(data);
    int r = calcParityBits(m);
    int n = m + r;

    int code[n + 1]; // 1-based index
    int j = 0;

    // Place data bits & leave parity slots blank
    for (int i = 1; i <= n; i++) {
        if ((i & (i - 1)) == 0) {
            code[i] = 0; // parity placeholder
        } else {
            code[i] = data[j++] - '0';
        }
    }

    // Calculate parity bits
    for (int i = 0; i < r; i++) {
        int pos = 1 << i;
        int parity = 0;
        for (int k = 1; k <= n; k++) {
            if (k & pos) parity ^= code[k];
        }
        code[pos] = parity;
    }

    // Build encoded string
    for (int i = 1; i <= n; i++)
        encoded[i - 1] = code[i] + '0';
    encoded[n] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    char msg[BUF_SIZE - HEADER_SIZE];
    printf("Enter binary message: ");
    scanf("%s", msg);

    char encoded[BUF_SIZE - HEADER_SIZE];
    hammingEncode(msg, encoded);

    printf("Sender: Original=%s Encoded=%s\n", msg, encoded);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    char buffer[BUF_SIZE];
    sprintf(buffer, "LEN=%ld|%s", strlen(msg), encoded);
    send(sockfd, buffer, strlen(buffer), 0);
    printf("Sender: Sent packet: %s\n", buffer);

    // Wait for ACK
    int n = read(sockfd, buffer, BUF_SIZE);
    buffer[n] = '\0';
    printf("Sender: Got ACK = %s\n", buffer);

    close(sockfd);
    return 0;
}


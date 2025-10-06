#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 2048

// Recompute parity for arbitrary length
int hammingCheck(char *encoded) {
    int n = strlen(encoded);
    int r = 0;
    while ((1 << r) <= n) r++;

    int code[n + 1];
    for (int i = 1; i <= n; i++)
        code[i] = encoded[i - 1] - '0';

    int errorPos = 0;
    for (int i = 0; i < r; i++) {
        int pos = 1 << i;
        int parity = 0;
        for (int k = 1; k <= n; k++) {
            if (k & pos) parity ^= code[k];
        }
        if (parity != 0) errorPos += pos;
    }
    return errorPos;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    char buffer[BUF_SIZE];
    int n = read(sockfd, buffer, BUF_SIZE);
    buffer[n] = '\0';
    printf("Receiver: Got packet: %s\n", buffer);

    char *encoded = strchr(buffer, '|') + 1;
    int errorPos = hammingCheck(encoded);

    if (errorPos == 0) {
        printf("Receiver: No error detected\n");
        send(sockfd, "No error", 8, 0);
    } else {
        printf("Receiver: Error detected at bit %d\n", errorPos);
        send(sockfd, "Error", 5, 0);
    }

    close(sockfd);
    return 0;
}


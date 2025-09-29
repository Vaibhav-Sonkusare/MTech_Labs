// gcc error_client.c -o error_client
// ./error_client <server_ip> <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 4096

// Calculate number of parity bits for message length m
int calcParityBits(int m) {
    int r = 0;
    while ((1 << r) < (m + r + 1)) r++;
    return r;
}

// Hamming encode arbitrary-length binary string `data` into `encoded`
void hammingEncode(const char *data, char *encoded) {
    int m = strlen(data);
    int r = calcParityBits(m);
    int n = m + r;
    int *code = (int*)malloc((n + 1) * sizeof(int)); // 1-based index
    if (!code) { perror("malloc"); exit(1); }

    int j = 0;
    for (int i = 1; i <= n; i++) {
        if ((i & (i - 1)) == 0) {
            code[i] = 0; // parity placeholder
        } else {
            code[i] = data[j++] - '0';
        }
    }

    for (int i = 0; i < r; i++) {
        int pos = 1 << i;
        int parity = 0;
        for (int k = 1; k <= n; k++)
            if (k & pos) parity ^= code[k];
        code[pos] = parity;
    }

    for (int i = 1; i <= n; i++) encoded[i - 1] = code[i] + '0';
    encoded[n] = '\0';
    free(code);
}

// Randomly flip one bit in msg (or do nothing)
void introduceError(char *msg) {
    int len = strlen(msg);
    if (len == 0) return;
    int flip = rand() % 2; // 0 = no error, 1 = error
    if (flip == 1) {
        int pos = rand() % len;
        msg[pos] = (msg[pos] == '0') ? '1' : '0';
        printf("Error Client: Introduced error at codeword position %d (1-based)\n", pos + 1);
    } else {
        printf("Error Client: No error introduced\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    srand((unsigned int)(time(NULL) ^ getpid()));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    char buffer[BUF_SIZE];
    int n = read(sockfd, buffer, BUF_SIZE - 1);
    if (n <= 0) {
        perror("read");
        close(sockfd);
        return 1;
    }
    buffer[n] = '\0';
    printf("Error Client: Got packet: %s\n", buffer);

    // Parse packet: optional "LEN=<m>|<payload>"
    char *pipe = strchr(buffer, '|');
    char *payload = NULL;
    int m = -1; // original message length
    int payload_len = 0;

    if (pipe && strncmp(buffer, "LEN=", 4) == 0) {
        // parse number between "LEN=" and '|'
        char numbuf[64];
        int numlen = (int)(pipe - (buffer + 4));
        if (numlen >= (int)sizeof(numbuf)) numlen = sizeof(numbuf) - 1;
        strncpy(numbuf, buffer + 4, numlen);
        numbuf[numlen] = '\0';
        m = atoi(numbuf);
        payload = pipe + 1;
    } else {
        // No LEN header: treat entire buffer as payload
        payload = buffer;
        m = (int)strlen(payload);
    }

    payload_len = (int)strlen(payload);
    int r = calcParityBits(m);
    int expected_code_len = m + r;

    char *codeword = NULL;
    char encoded_buf[BUF_SIZE];

    if (payload_len == expected_code_len) {
        // payload already a codeword
        codeword = strdup(payload);
        if (!codeword) { perror("strdup"); close(sockfd); return 1; }
        printf("Error Client: Payload appears to be already encoded (len=%d).\n", payload_len);
    } else if (payload_len == m) {
        // payload is raw data; encode it
        hammingEncode(payload, encoded_buf);
        codeword = strdup(encoded_buf);
        if (!codeword) { perror("strdup"); close(sockfd); return 1; }
        printf("Error Client: Encoded raw payload (%d bits) -> codeword (len=%d): %s\n", m, (int)strlen(codeword), codeword);
    } else {
        // Ambiguous lengths: fallback -> treat as raw data and encode
        printf("Error Client: Unexpected payload length (%d). Expected data len=%d or codeword len=%d.\n",
               payload_len, m, expected_code_len);
        // attempt to treat payload as raw data and encode whole payload
        hammingEncode(payload, encoded_buf);
        codeword = strdup(encoded_buf);
        if (!codeword) { perror("strdup"); close(sockfd); return 1; }
        m = (int)strlen(payload); // update m to the actual data length used
        r = calcParityBits(m);
        expected_code_len = m + r;
        printf("Error Client: After fallback encoding -> codeword len=%d: %s\n", expected_code_len, codeword);
    }

    // Introduce error or not
    introduceError(codeword);

    // Build packet to forward (preserve "LEN=" if was present)
    char outbuf[BUF_SIZE];
    if (pipe && strncmp(buffer, "LEN=", 4) == 0)
        snprintf(outbuf, sizeof(outbuf), "LEN=%d|%s", m, codeword);
    else
        snprintf(outbuf, sizeof(outbuf), "%s", codeword);

    // Forward modified packet
    if (send(sockfd, outbuf, strlen(outbuf), 0) < 0) {
        perror("send");
        free(codeword);
        close(sockfd);
        return 1;
    }
    printf("Error Client: Forwarded packet: %s\n", outbuf);

    free(codeword);
    close(sockfd);
    return 0;
}


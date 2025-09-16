#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5000
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 10

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    int length = 5;
    int data[1 + 2 * length];
    data[0] = length;
    printf("Sending two arrays of even numbers...\n");

    for (int i = 0; i < length; i++) {
        data[1 + i] = (i + 1) * 2;        // array1
        data[1 + length + i] = (i + 2) * 2; // array2
    }

    int attempts = 0;
    while (attempts < 3) {
        sendto(sockfd, data, sizeof(data), 0,
               (struct sockaddr *)&server_addr, addr_len);

        printf("Packet sent, waiting for reply...\n");

        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&server_addr, &addr_len);
        if (n < 0) {
            perror("Timeout or error, retransmitting...");
            attempts++;
            continue;
        }

        int *resp = (int *)buffer;
        int resp_len = resp[0];
        printf("Received response with length %d:\n", resp_len);
        for (int i = 0; i < resp_len; i++) {
            printf("%d ", resp[i + 1]);
        }
        printf("\n");
        break;
    }

    if (attempts == 3) {
        printf("Failed to receive response after 3 attempts.\n");
    }

    close(sockfd);
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&client_addr, &addr_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        int *data = (int *)buffer;
        int length = data[0];
        if (length <= 0 || 2 * length + 1 > n / sizeof(int)) {
            printf("Invalid data size received.\n");
            continue;
        }

        int *array1 = &data[1];
        int *array2 = &data[1 + length];
        int result[length];
        int valid = 1;

        for (int i = 0; i < length; i++) {
            if (array1[i] % 2 != 0 || array2[i] % 2 != 0) {
                valid = 0;
                break;
            }
        }

        if (!valid) {
            printf("Invalid data detected (odd numbers present). Dropping packet.\n");
            continue;
        }

        for (int i = 0; i < length; i++) {
            result[i] = array1[i] + array2[i];
        }

        int response_size = sizeof(int) * (1 + length);
        int response[1 + length];
        response[0] = length;
        for (int i = 0; i < length; i++) {
            response[i + 1] = result[i];
        }

        sendto(sockfd, response, response_size, 0,
               (struct sockaddr *)&client_addr, addr_len);
        printf("Response sent.\n");
    }

    close(sockfd);
    return 0;
}


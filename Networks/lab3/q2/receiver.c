#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 1024

int check_hamming(char *packet) {
    int n = strlen(packet);
    int r = 0;
    while ((1 << r) <= n) r++;

    int error_pos = 0;
    for (int i = 0; i < r; i++) {
        int pos = 1 << i;
        int parity = 0;
        for (int k = 1; k <= n; k++) {
            if (k & pos) {
                parity ^= (packet[k-1] - '0');
            }
        }
        if (parity != 0) error_pos += pos;
    }
    return error_pos; // 0 means no error
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[MAX];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    while (1) {    memset(buffer, 0, MAX);
        read(sock, buffer, MAX);
        printf("Receiver got: %s\n", buffer);

        char *msg = strstr(buffer, "MSG:") + 4;
        int error = check_hamming(msg);

        char ack[50];
        if (error == 0) {
            sprintf(ack, "No error");
        } else {
            sprintf(ack, "Error");
        }

        printf("Receiver sent ACK: %s\n", ack);
        write(sock, ack, strlen(ack));

        if (error == 0) {
            break;
        }
    }

    close(sock);
    return 0;
}


// sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 1024

unsigned short checksum(unsigned short *msg, int nwords) {
    unsigned long sum = 0;

    for (int i = 0; i < nwords; i++) {
        sum += msg[i];
    }

    // Add carries until no carry remains
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (unsigned short)(~sum);
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[MAX];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    int n;
    printf("Enter number of 16-bit words: ");
    scanf("%d", &n);

    unsigned short msg[n];
    printf("Enter %d 16-bit hex values: \n", n);
    for (int i = 0; i < n; i++) {
        scanf("%hx", &msg[i]);
    }

    unsigned short cs = checksum(msg, n);
    printf("DEBUG: Calculated checksum = %04x\n", cs);

    sprintf(buffer, "DATA:");
    for (int i = 0; i < n; i++) {
        char temp[10];
        sprintf(temp, "%04x ", msg[i]);
        strcat(buffer, temp);
    }
    char temp[10];
    sprintf(temp, "CHK:%04x", cs);
    strcat(buffer, temp);

    write(sock, buffer, strlen(buffer));
    printf("Sender sent: %s\n", buffer);

    memset(buffer, 0, MAX);
    read(sock, buffer, MAX);
    printf("Sender got ACK: %s\n", buffer);

    close(sock);
    return 0;
}


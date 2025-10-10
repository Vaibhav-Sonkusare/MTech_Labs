// receiver.c
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
        if (sum & 0x10000) {
            sum = (sum & 0xFFFF) + 1;
        }
    }
    return ~sum;
}

unsigned short verify_checksum(unsigned short *msg, int nwords) {
    unsigned long sum = 0;

    for (int i = 0; i < nwords; i++) {
        sum += msg[i];
    }

    // fold carries
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (unsigned short)sum;  // should be 0xFFFF if OK
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

    memset(buffer, 0, MAX);
    read(sock, buffer, MAX);
    printf("Receiver got: %s\n", buffer);

    // Extract data
    unsigned short msg[50];
    int count = 0;
    char *token = strtok(buffer, " ");
    while (token != NULL) {
        if (strncmp(token, "DATA:", 5) == 0) {
            token += 5;
            msg[count++] = (unsigned short)strtol(token, NULL, 16);
        } else if (strncmp(token, "CHK:", 4) == 0) {
            token += 4;
            msg[count++] = (unsigned short)strtol(token, NULL, 16);
        } else {
            msg[count++] = (unsigned short)strtol(token, NULL, 16);
        }
        token = strtok(NULL, " ");
    }

    unsigned short cs = verify_checksum(msg, count);
    char result[50];
    if (cs == 0xFFFF) {
        sprintf(result, "No Error");
    } else {
        sprintf(result, "Error");
    }


    printf("Receiver sent ACK: %s\n", result);
    write(sock, result, strlen(result));

    close(sock);
    return 0;
}


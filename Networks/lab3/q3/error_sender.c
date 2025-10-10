// error_sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 1024

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
    printf("Error Sender got: %s\n", buffer);

    // Seed random generator
    srand(time(NULL));

    // Decide randomly: 50% chance to induce error
    int induce = rand() % 2;

    if (induce) {
        // Pick a random index within the message string (excluding null terminator)
        int len = strlen(buffer);
        int pos = rand() % len;

        // Flip character (simple change: if hex digit, flip to another)
        if (buffer[pos] >= '0' && buffer[pos] <= '9') {
            buffer[pos] = (buffer[pos] == '9') ? '0' : buffer[pos] + 1;
        } else if (buffer[pos] >= 'a' && buffer[pos] <= 'f') {
            buffer[pos] = (buffer[pos] == 'f') ? 'a' : buffer[pos] + 1;
        } else if (buffer[pos] >= 'A' && buffer[pos] <= 'F') {
            buffer[pos] = (buffer[pos] == 'F') ? 'A' : buffer[pos] + 1;
        } else {
            // fallback: toggle case
            buffer[pos] ^= 0x20;
        }

        printf("Error induced at position %d. Modified data: %s\n", pos, buffer);
    } else {
        printf("No error induced. Data unchanged.\n");
    }

    write(sock, buffer, strlen(buffer));
    close(sock);
    return 0;
}


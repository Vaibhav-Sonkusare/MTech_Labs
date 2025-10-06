// error_sender.c
#include <stdio.h>
#include <stdlib.h>
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

    // Introduce error (flip one bit in first word)
    buffer[5] = (buffer[5] == 'f') ? '0' : 'f'; 

    printf("Error Sender modified data: %s\n", buffer);
    write(sock, buffer, strlen(buffer));

    close(sock);
    return 0;
}


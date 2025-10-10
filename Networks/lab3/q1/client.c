#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

void *readThread(void *arg) {
    int sock = *(int *)arg;
    char buf[256];
    while (1) {
        int n = read(sock, buf, sizeof(buf)-1);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("%s", buf);
    }
    return NULL;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    pthread_t tid;
    pthread_create(&tid, NULL, readThread, &sock);

    char bid[32];
    while (1) {
        fgets(bid, sizeof(bid), stdin);
        write(sock, bid, strlen(bid));
    }
}

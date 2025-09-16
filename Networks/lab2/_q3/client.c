#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

int main() {
    
    int sockfd = socket(AF_INET, SOCK_STREAM ,0);

    if(sockfd < 0) {
        printf("Socket Creation Error\n");
        return - 1;
    }

    printf("[+] Socket is Created\n\n");

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(8800);
    sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    socklen_t sock_len = sizeof(sock_addr);
    
    int connectfd = connect(sockfd, (struct sockaddr*) &sock_addr, sock_len);

    if(connectfd == -1) {
        printf("Connecting Error\n");
        return -1;
    }

    char msg[100];
    int n = read(sockfd, msg, 100);
    msg[n] = '\0';
    bool is_over = false;

    printf("%s", msg);

    while(1) {
        char msg2[100];
        int n = read(sockfd, msg2, 100);
        
        if(msg2[n - 1] == '#') {
            is_over = true;
            msg2[n - 1] = '\0';
        }
        else {
            msg2[n] = '\0';
        }


        printf("%s", msg2);
        fflush(stdout);

        if(is_over) break;

        int first, second;

        printf("\nEnter Row : ");
        scanf("%d", &first);


        printf("Enter Column : ");
        scanf("%d", &second);

        int user_input[2] = {first, second};

        write(sockfd, user_input, sizeof(user_input));
    }

    close(sockfd);

    return 0;
}

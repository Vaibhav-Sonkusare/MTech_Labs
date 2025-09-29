#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

int sockfd;
int board[3][3] = {{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}};
int first_player;

void updateBoard() {
    FILE *fp = fopen("board.txt", "w");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file for writing");
        return;
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == -1) {
                fprintf(fp, " _ ");
            }
            else if (board[i][j] == 1) {
                fprintf(fp, " X ");
            }
            else if (board[i][j] == 0) {
                fprintf(fp, " O ");
            }
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");

    fclose(fp);
}

void printBoard() {
    for(int i = 0;i < 3; i++) {
        for(int j = 0;j < 3; j++) {
            if(board[i][j] == -1) {
                printf(" _ ");
            }
            else if(board[i][j] == 1) {
                printf(" X ");
            }
            else if(board[i][j] == 0) {
                printf(" O ");
            }
        }

        printf("\n");
    }
    printf("\n");
    updateBoard();
}

bool checkWon(int current_player) {
    int data;

    if(current_player == first_player) data = 1;
    else data = 0;

    //Check Rows and Columns
    for(int i = 0;i < 3; i++) {
        bool isValidRow = true;
        bool isValidCol = true;
            
        for(int j = 0;j < 3; j++) {
            if(board[i][j] != data) isValidRow = false;

            if(board[j][i] != data) isValidCol = false;
        }

        if(isValidRow || isValidCol) return true;
    }

    bool leftDiagonalValid = true;
    bool rightDiagonalValid = true;
    //Check Diagonals
    for(int i = 0;i < 3; i++) {    
        if(board[i][i] != data) leftDiagonalValid = false;
        
        if(board[i][2 - i] != data) rightDiagonalValid = false;
    }

    if(leftDiagonalValid || rightDiagonalValid) return true;

    return false;
}

void gameOver(int current_player, int second_player) {
	char msg[] = "#########";
    write(current_player, msg, strlen(msg));
    write(second_player, msg, strlen(msg));
    
    char buffer[100];
	
    char msg1[] = "Congrats You Won\nGame Over\n";
    memset(buffer, '\0', sizeof(buffer));
    strncpy(buffer, msg1, strlen(msg1));
    write(current_player, buffer, strlen(buffer));

    char msg2[] = "You Lose\nGame Over\n";
    memset(buffer, '\0', sizeof(buffer));
    strncpy(buffer, msg2, strlen(msg2));
    write(second_player, buffer, strlen(buffer));
}


void playGame(int current_player, int second_player) {
    
    while(1) {
        //PrinBoard
        printBoard();

        char msg[] = "Your Turn";
        write(current_player, msg, strlen(msg));

        int data[2];
        read(current_player, data, sizeof(data));

        int first = data[0] - 1;
        int second = data[1] - 1;

        if(first_player == current_player) { 
            board[first][second] = 1;
        }
        else {
            board[first][second] = 0; 
        }
        //Check Win Condition
        bool didWin = checkWon(current_player);

        if(didWin) {
            printBoard();
            gameOver(current_player, second_player);
            break;
        }

        int temp = current_player;
        current_player = second_player;
        second_player = temp;
    }
}

void handleClient(int client_fd1, int client_fd2) {

    int current_player;
    int second_player;
    //Random Select First Player
    int random_num = rand() % 2;

    if(random_num == 0) {
        current_player = client_fd1;
        second_player = client_fd2;
    }
    else {
        current_player = client_fd2;
        second_player = client_fd1;
    }
    first_player = current_player;

    
    char message1[] = "You are 'X'\nYou will play first\n";
    write(current_player, message1, strlen(message1));

    char message2[] = "You are 'O'\nYou will play second\n";
    write(second_player, message2, strlen(message2));

    playGame(current_player, second_player);

    close(current_player);
    printf("Client %d Disconnected\n", current_player);

    close(second_player);
    printf("Client %d Disconnected\n", second_player);
}


int main() {
    sockfd = socket(AF_INET, SOCK_STREAM ,0);

    if(sockfd < 0) {
        printf("Socket Creation Error\n");
        return - 1;
    }

    printf("[+] Socket is Created\n");

    struct sockaddr_in sock_addr;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(8800);
    sock_addr.sin_addr.s_addr = INADDR_ANY;

    socklen_t sock_len = sizeof(sock_addr);

    int bindfd = bind(sockfd, (struct sockaddr *)&sock_addr, sock_len);

    if(bindfd < 0) {
        printf("Bind Failed\n");
        return -1;
    }

    printf("[+] Binded With IP Address and Port\n\n");

    //Max 2 requests in queue
    int listenfd = listen(sockfd, 2);

    if(listenfd < 0) {
        printf("Listen Error\n");
        return -1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd1 = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
    
    if(client_fd1 < 0) {
        printf("Problem while accept\n");
        return -1;
    };

    printf("Client %d Connected\n", client_fd1);

    int client_fd2 = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);

    if(client_fd2 < 0) {
        printf("Problem while accept\n");
        return -1;
    };

    printf("Client %d Connected\n", client_fd2);

    handleClient(client_fd1, client_fd2);

    close(sockfd);

    return 0;
}

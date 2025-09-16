#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

void printBoard();
bool checkWin(int);

int sockfd;
int board[3][3] = {{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}};
int first_player;

int main() {
    sockfd = socket(AF_INET, SOCK_STREAM ,0);

    if(sockfd < 0) {
        printf("Socket Creation Error\n");
        return - 1;
    }

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

	int current_player;
    int second_player;
    
    // Do a toss
    printf("Making a toss!\n");
    int random_num = rand() % 2;
    printf("X Plays first.\n");

    if(random_num == 0) {
        current_player = client_fd1;
        second_player = client_fd2;
    }
    else {
        current_player = client_fd2;
        second_player = client_fd1;
    }
    first_player = current_player;

    
    char message1[] = "You are 'X'\nYou won the toss\nYou will play first\n";
    write(current_player, message1, strlen(message1));

    char message2[] = "You are 'O'\nYou lost the toss\nYou will play second\n";
    write(second_player, message2, strlen(message2));

	// Game Loop
    while(1) {
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
        if(checkWin(current_player)) {
            // Game Over
            printBoard();
            
            char msg[] = "Congrats You Won\nGame Over\n#";
			write(current_player, msg, strlen(msg));

			char msg2[] = "You Lose\nGame Over\n#";
			write(second_player, msg2, strlen(msg2));
            break;
        }

        int temp = current_player;
        current_player = second_player;
        second_player = temp;
    }

    close(first_player);
    close(second_player);
    close(sockfd);

    return 0;
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
}

bool checkWin(int current_player) {
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


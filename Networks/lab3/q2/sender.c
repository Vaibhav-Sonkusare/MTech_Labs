// sender.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 1024

// Function to insert parity bits (Hamming code)
void create_hamming_packet(char *data, int m_len, char *packet) {
    int r = 0; // number of parity bits
    while ((1 << r) < (m_len + r + 1)) r++;  // calculate parity bits count

    int n = m_len + r;  // total length of packet
    int j = 0;
    for (int i = 1; i <= n; i++) {
        if ((i & (i - 1)) == 0) { // power of 2 → parity bit
            packet[i] = '0'; // placeholder
        } else {
            packet[i] = data[j++];
        }
    }

    // Compute parity bits
    for (int i = 0; i < r; i++) {
        int pos = 1 << i;
        int parity = 0;
        for (int k = 1; k <= n; k++) {
            if (k & pos) {
                parity ^= (packet[k] - '0');
            }
        }
        packet[pos] = parity + '0';
    }
    packet[n+1] = '\0'; // null terminate
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[MAX], packet[50];
    char msg[50];
    int msg_len;

    // --- Connect to server ---
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
    
    // --- Get user input ---
    printf("Enter message length: ");
    scanf("%d", &msg_len);

    if (msg_len <= 0 || msg_len >= 50) {
        printf("Invalid message length!\n");
        return 1;
    }

    printf("Enter message bits (length %d): ", msg_len);
    scanf("%s", msg);

    if (strlen(msg) != msg_len) {
        printf("Message length does not match input length!\n");
        return 1;
    }

    // --- Build Hamming packet ---
    create_hamming_packet(msg, msg_len, packet);


    while (1) {    // --- Send packet ---
        sprintf(buffer, "LEN:%d MSG:%s", msg_len, packet+1); // +1 because Hamming uses 1-based indexing
        printf("Sender sending: %s\n", buffer);
        write(sock, buffer, strlen(buffer));

        // --- Wait for ACK ---
        memset(buffer, 0, MAX);
        read(sock, buffer, MAX);
        printf("Sender got ACK: %s\n", buffer);

        if (strncmp(buffer, "Error", 5) != 0) {
            break;
        }
    }

    close(sock);
    return 0;
}

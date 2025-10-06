#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 12345
#define BUFFER_SIZE 1024

double evaluate_expression(const char *expr) {
    double result = 0;
    double current = 0;
    char op = '+';
    const char *p = expr;

    while (*p) {
        // Skip whitespace
        while (isspace(*p)) p++;

        // Read number
        if (sscanf(p, "%lf", &current) != 1) {
            return 0; // Invalid input
        }

        // Apply previous operator
        switch (op) {
            case '+': result += current; break;
            case '-': result -= current; break;
            case '*': result *= current; break;
            case '/': 
                if (current == 0) return 0; // divide by zero
                result /= current; 
                break;
        }

        // Move pointer past the number
        while (*p && (isdigit(*p) || *p == '.' || *p == '-')) p++;

        // Skip whitespace
        while (isspace(*p)) p++;

        // Read next operator
        if (*p) {
            op = *p;
            p++;
        }
    }

    return result;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Math server listening on port %d...\n", PORT);

    // Accept one client
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    printf("Client connected.\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) break; // Client disconnected

        if (strncmp(buffer, "exit", 4) == 0) break; // Terminate server on client exit

        double result = evaluate_expression(buffer);
        char result_str[BUFFER_SIZE];
        snprintf(result_str, sizeof(result_str), "%lf", result);
        send(new_socket, result_str, strlen(result_str), 0);
    }

    printf("Client disconnected. Shutting down server.\n");
    close(new_socket);
    close(server_fd);
    return 0;
}


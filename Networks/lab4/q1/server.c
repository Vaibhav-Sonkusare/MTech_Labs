#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdbool.h>

#define PORT 12345
#define BUFFER_SIZE 1024

typedef struct {
    const char *p;
    bool error;
} Parser;

void skip_spaces(Parser *ps) {
    while (isspace(*ps->p)) ps->p++;
}

// Forward declarations
double parse_expr(Parser *ps);
double parse_term(Parser *ps);
double parse_factor(Parser *ps);

// Parse a number or a parenthesized expression, including unary minus
double parse_factor(Parser *ps) {
    skip_spaces(ps);

    double result = 0.0;

    // Handle unary plus/minus
    if (*ps->p == '+' || *ps->p == '-') {
        char sign = *ps->p;
        ps->p++;
        double value = parse_factor(ps);
        result = (sign == '-') ? -value : value;
        return result;
    }

    // Parenthesized subexpression
    if (*ps->p == '(') {
        ps->p++; // skip '('
        result = parse_expr(ps);
        skip_spaces(ps);

        if (*ps->p == ')') {
            ps->p++; // skip ')'
        } else {
            fprintf(stderr, "Error: missing closing parenthesis\n");
            ps->error = true;
            return 0;
        }
        return result;
    }

    // Must be a number
    if (!isdigit(*ps->p) && *ps->p != '.') {
        fprintf(stderr, "Error: expected number but found '%c'\n", *ps->p);
        ps->error = true;
        return 0;
    }

    char *endptr;
    result = strtod(ps->p, &endptr);
    ps->p = endptr;
    return result;
}

// Parse * and / operations
double parse_term(Parser *ps) {
    double result = parse_factor(ps);
    skip_spaces(ps);

    while (*ps->p == '*' || *ps->p == '/') {
        char op = *ps->p;
        ps->p++;
        double rhs = parse_factor(ps);

        if (op == '*') result *= rhs;
        else {
            if (rhs == 0) {
                fprintf(stderr, "Error: divide by zero\n");
                ps->error = true;
                return 0;
            }
            result /= rhs;
        }

        skip_spaces(ps);
    }

    return result;
}

// Parse + and - operations
double parse_expr(Parser *ps) {
    double result = parse_term(ps);
    skip_spaces(ps);

    while (*ps->p == '+' || *ps->p == '-') {
        char op = *ps->p;
        ps->p++;
        double rhs = parse_term(ps);
        if (op == '+') result += rhs;
        else result -= rhs;
        skip_spaces(ps);
    }

    return result;
}

// Public entry point
double evaluate_expression(const char *expr) {
    Parser ps = {expr, false};
    double result = parse_expr(&ps);

    skip_spaces(&ps);
    if (*ps.p != '\0') {
        fprintf(stderr, "Error: unexpected character '%c'\n", *ps.p);
        ps.error = true;
    }

    return ps.error ? 0.0 : result;
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


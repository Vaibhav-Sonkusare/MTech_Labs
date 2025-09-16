/* This code runs an iterative server on localhost.
 * Responsibilities of an iterative server:
 *     ➢ The server waits for incoming client requests.
 *     ➢ Upon receiving a request:
 *         • It processes the request by providing the required service to the client.
 *         • Notifies the client once the service is completed.
 *     ➢ While processing a client’s request:
 *         • Either no other client requests are handled simultaneously,
 *         OR
 *         • Incoming requests are queued in a buffer.
 *     ➢ After completing the current service:
 *         • Either new request is accepted
 *         OR
 *         • The next request is taken from the buffer and processed.
 */

#include <stdio.h>
#include <stdlib.h>             // for {m,c}alloc syscall
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>             // for close syscall
#include <ctype.h>              // for isspace(), isdigit(), etc.

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define LISTEN_BACKLOG 5
#define BUFFER_SIZE 1024
#define MAX_STACK 1024
#define ERROR_EVALUATION -1234

struct device {
    int fd;
    struct sockaddr_in addr;
};

struct stack {
    int data[MAX_STACK];
    int top;
};

// Client and server initialization and cleanup
struct device *initialize_server();
struct device *accept_client(struct device *);
void cleanup_device(struct device *);

// Postfix Expression read and evaluation of postfix expression
char *read_postfix_exp(struct device *);
int eval_postfix_exp(char *);

// struct stack operations 
void stack_init(struct stack *);
void push(struct stack *, int);
int pop(struct stack *);


int main(int argc, char **argv) {
    struct device *server = initialize_server();
    printf("Server initialized.\n");

    while (1) {
        struct device *client = accept_client(server);
        printf("Client connected.\n");
        if (client == NULL) continue;

        // Get postfix expression from client
        char *data = read_postfix_exp(client);
        if (data != NULL) {
            printf("\n");
            printf("Expression from client: %s\n", data);
            int res = eval_postfix_exp(data);
            printf("Expression Result = %d\n", res);

            // sending result back to client
            write(client->fd, &res, sizeof(res));

            free(data);
        } else {
            fprintf(stderr, "main: error reading data\nCleaning up...\n");
        }

        cleanup_device(client);
    } 

    return 0;
}

struct device *initialize_server() {
    struct device *server = calloc(1, sizeof(struct device));
    if (server == NULL) {
        perror("initilize_server: calloc failed");
        exit(EXIT_FAILURE);
    }

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->fd == -1) {
        perror("initialize_connection: Socket Creation Failed!");
        exit(EXIT_FAILURE);
    }

    server->addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDRESS, &server->addr.sin_addr);
    server->addr.sin_port = htons(PORT_NO);
    
    if (bind(server->fd, (struct sockaddr *) &server->addr, sizeof(server->addr)) == -1) {
        perror("initialize_connection: bind Failed!");
        close(server->fd);
        free(server);
        exit(EXIT_FAILURE);
    }

    if (listen(server->fd, LISTEN_BACKLOG) == -1) {
        perror("initialize_connection: listen Failed!");
        close(server->fd);
        free(server);
        exit(EXIT_FAILURE);
    }

    return server;
}

struct device *accept_client(struct device *server) {
    struct device *client = calloc(1, sizeof(struct device));
    if (client == NULL) {
        perror("accept_client: calloc failed");
        return NULL;
    }
    char *data = calloc(BUFFER_SIZE, sizeof(char));
    if (data == NULL) {
        perror("accept_client: calloc failed");
        free(client);
        return NULL;
    }

    socklen_t client_sockaddr_in_size = sizeof(client->addr);
    client->fd = accept(server->fd, (struct sockaddr*) &client->addr, &client_sockaddr_in_size);

    if (client->fd == -1) {
        perror("Error Connecting Client");
        free(client);
        return NULL;
    }

    return client;
}


// Close the client file descriptor by using close syscall and free associated memory
void cleanup_device(struct device *client) {
    if (close(client->fd) == -1) {
        perror("cleanup_device: unable to close file descriptor");
    }
    free(client);
}

char *read_postfix_exp(struct device *client) {
    char *data = calloc(BUFFER_SIZE, sizeof(char));
    if (data == NULL) {
        perror("read_postfix_exp: error in calloc");
        return NULL;
    }

    ssize_t bytes_read = read(client->fd, data, BUFFER_SIZE);
    if (bytes_read == -1) {
        perror("Error reading data from client.");
        free(data);
        return NULL;
    }

    return data;
}

int eval_postfix_exp(char *data) {
    struct stack s;
    stack_init(&s);

    int i=0;
    int n = strlen(data);
    if (n > BUFFER_SIZE) {
        fprintf(stderr, "Invalid postfix string!\n");
        fprintf(stderr, "postfix string len = %d > BUFFER_SIZE(%d)\n", n, BUFFER_SIZE);
        return ERROR_EVALUATION;
    }

    while (i < n)
    {
        // Skip spaces
        if (isspace(data[i])) {
            i++;
            continue;
        }

        // Digit
        if (isdigit(data[i]) || (data[i] == '-' && (i + 1 < n) && isdigit(data[i + 1]))) {
            int sign = 1;
            if (data[i] == '-') {
                sign = -1;
                i++;
            }

            int num = 0;
            while (i < n && isdigit(data[i])) {
                num = num * 10 + (data[i] - '0');
                i++;
            }

            num *= sign;
            push(&s, num);
        } else {                    // Operator
            if (s.top < 1) {
                fprintf(stderr, "Invalid expression\n");
                return ERROR_EVALUATION;
            }

            int b = pop(&s);
            int a = pop(&s);
            int result = 0;

            switch (data[i]) {
                case '+': result = a + b; break;
                case '-': result = a - b; break;
                case '*': result = a * b; break;
                case '/':
                    if (b == 0) {
                        fprintf(stderr, "Division by zero error\n");
                        return ERROR_EVALUATION;
                    }
                    result = a / b;
                    break;
                default:
                    fprintf(stderr, "Unknown operator: %c\n", data[i]);
                    return ERROR_EVALUATION;
            }
            push(&s, result);
            i++;
        }
    }
    
    if (s.top != 0) {
        fprintf(stderr, "Invalid expression\n");
        return ERROR_EVALUATION;
    }
    return pop(&s);
}

void stack_init(struct stack *s) {
    s->top = -1;
}

void push(struct stack *s, int val) {
    if (s->top < MAX_STACK - 1) {
        s->data[++(s->top)] = val;
    } else {
        fprintf(stderr, "Stack overflow\n");
        return;
    }
}

int pop(struct stack *s) {
    if (s->top >= 0) {
        return s->data[(s->top)--];
    } else {
        fprintf(stderr, "Stack underflow\n");
        return ERROR_EVALUATION;
    }
}
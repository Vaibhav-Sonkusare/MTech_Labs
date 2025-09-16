#include <stdio.h>
#include <stdlib.h> // for {m,c}alloc syscall
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> // for close syscall

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

// Client initialization and cleanup 
struct device *initialize_client();
void cleanup_device(struct device *);

int main(int argc, char **argv) {
	while (1) {
		char data[BUFFER_SIZE];
		
		printf("Please enter a postfix expression: ");
		if (fgets(data, BUFFER_SIZE, stdin) == NULL) {
			fprintf(stderr, "main: error in fgets\n"); 
		} else { 
			struct device *client = initialize_client();
			write(client->fd, data, BUFFER_SIZE);
			printf("Expression send to server.");
			printf("Waiting for reply...");
			
			int res;
			ssize_t size_res = read(client->fd, &res, sizeof(res));
			if (size_res != sizeof(res)) { 
				fprintf(stderr, "main: invalid data read from server\n");
			} else { 
				printf("\nResult = %d\n", res);
			}
			
			cleanup_device(client);
		}
	}
	
	return 0;
}

struct device *initialize_client() {
	struct device *client = calloc(1, sizeof(struct device));
	if (client == NULL) { 
		perror("initialize_client: calloc failed");
		exit(EXIT_FAILURE);
	}
	
	client->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client->fd == -1) { 
		perror("initialize_client: error creating socket");
		exit(EXIT_FAILURE);
	}
	
	client->addr.sin_family = AF_INET;
	inet_pton(AF_INET, IP_ADDRESS, &client->addr.sin_addr);
	client->addr.sin_port = htons(PORT_NO);
	
	if (connect(client->fd, (struct sockaddr *) &client->addr, sizeof(client->addr)) == -1) {
		perror("initialize_client: error in connect()");
		exit(EXIT_FAILURE);
	}
	
	return client;
}

void cleanup_device(struct device *client) {
	if (close(client->fd) == -1) {
		perror("cleanup_device: unable to close file descriptor");
	}
	free(client);
}







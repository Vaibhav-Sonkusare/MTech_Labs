#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IP_ADDRESS "127.0.0.1" 
#define PORT_NO 5000
#define BACKLOG 0
#define READ_BUFFER_SIZE 1024
#define CHAR_BUFFER_SIZE 2

int main(int argc, char **argv) {
	int socket_fd;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		printf("Error: Creating Socket!\n");
		return -1;
	}

	struct sockaddr_in myaddr;
	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	inet_pton(AF_INET, IP_ADDRESS, &myaddr.sin_addr);
	myaddr.sin_port = htons(PORT_NO);

	if (connect(socket_fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
		printf("Error: Connection Error!\n");
		close(socket_fd);
		return -1;
	}

	printf("Successfully connected to Server.\n");
	printf("Sending data to Server.\n");
	char char_buffer[READ_BUFFER_SIZE];
	char *data = "AHeallo";
	memset(char_buffer, '\0', READ_BUFFER_SIZE);
	strncpy(char_buffer, data, strlen(data));
	write(socket_fd, char_buffer, (size_t) READ_BUFFER_SIZE);
	printf("Data sent to Server: %s\n", char_buffer);
	

	close(socket_fd);
	
}

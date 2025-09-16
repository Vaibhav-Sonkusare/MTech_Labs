#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define IP_ADDRESS "127.0.0.1" 
#define PORT_NO 5000
#define BACKLOG 5
#define READ_BUFFER_SIZE 1024
#define CHAR_BUFFER_SIZE 2

int main(int argc, char **argv) {
	int socket_fd, client1_fd, client2_fd;
	
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		printf("Error: Creating Socket!\n");
		return -1;
	}

	struct sockaddr_in my_addr, peer_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	inet_pton(AF_INET, IP_ADDRESS, &my_addr.sin_addr);
	my_addr.sin_port = htons(PORT_NO);
	
	if (bind(socket_fd, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {
		printf("Error: Caling bind!\n");
		close(socket_fd);
		return -1;
	}

	if (listen(socket_fd, BACKLOG) == -1) {
		printf("Error: Calling listen!\n");
		close(socket_fd);
		return -1;
	}

	socklen_t peer_addr_size = sizeof(peer_addr);
	client1_fd = accept(socket_fd, (struct sockaddr *) &peer_addr, &peer_addr_size);
	printf("Successfully Connected to client 1\n");
	client2_fd = accept(socket_fd, (struct sockaddr *) &peer_addr, &peer_addr_size);
	printf("Successfully Connected to client 2\n");
	
	// Receiving data from client1
	char char_buffer[CHAR_BUFFER_SIZE];
	ssize_t bytes_read = read(client1_fd, char_buffer, CHAR_BUFFER_SIZE);
	char_buffer[1] = '\0';
	printf("Data Received form client 1: %s\n", char_buffer);
	
	// Decrementing char_buffer[0]
	char_buffer[0] -= 1;
	
	// Sending data to client2
	write(client2_fd, char_buffer, (size_t) CHAR_BUFFER_SIZE);
	printf("Data sent to client 2: %s\n", char_buffer);

	printf("Closing connection.\n");
	close(client1_fd);
	close(client2_fd);
	close(socket_fd);

	return 0;
}

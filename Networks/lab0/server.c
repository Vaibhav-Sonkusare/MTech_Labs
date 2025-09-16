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

int main(int argc, char **argv) {
	int socket_fd, client_fd;
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
	client_fd = accept(socket_fd, (struct sockaddr *) &peer_addr, &peer_addr_size);

	printf("Successfully Connected to client.\n");
	char *to_send = "Hi Client! Nice to meet you!";
	printf("Sending data: %s\n", to_send);
	write(client_fd, to_send, (size_t) strlen(to_send) + 4);
	printf("Waiting to receive data from client...\n");
	/*char buffer[READ_BUFFER_SIZE];
	read(client_fd, buffer, READ_BUFFER_SIZE);
	buffer[READ_BUFFER_SIZE - 1] = '\0';
	printf("Received data from client: %s\n");*/

	printf("Closing connection.\n");
	close(client_fd);
	close(socket_fd);

	return 0;
}

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
#define BACKLOG 2
#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
	int socket_fd, client_fd;
	
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
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
	
	socklen_t peer_addr_size = sizeof(peer_addr);
	char buffer[BUFFER_SIZE];
	
	printf("Server Ready!\n");
	
	while(1) {
		int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_size);
		
		if (n < 0) {
			perror("recvfrom failed");
			continue;
		}
		
		int *data = (int *)buffer;
		int length = data[0];
		if (length <= 0 || 2 * length + 1 > n / sizeof(int)) {
			printf("Invalid data size received.\n");
			continue;
		}
		
		int *array1 = &data[1];
		int *array2 = %data[1 + lenght];
		int result[lenght];
		
		int valid = 1;
		
		// Checking if all array1 and array2 elements are even
		for (int i=0; i< lenght; i++) {
			if (array1[i] % 2 != 0 || array2[i] % 2 != 0) {
				valid = 0;
				break;
			}
		}
		
		if (valid == 0) {
			printf("Invalid data detected!\nDropping packet.\n");
			continue;
		}
		
		for (int i=0; i< lenght; i++) {
			result[i] = array1[i] + array2[i];
		}
		
		int response_size = sizeof(int *) * (1 + lenght);
		int response[1 + lenght];
		response[0] = length;
        for (int i = 0; i < length; i++) {
            response[i + 1] = result[i];
        }

        sendto(sockfd, response, response_size, 0,
               (struct sockaddr *)&client_addr, addr_len);
        printf("Response sent.\n");
        break;
	}

	printf("Closing connection.\n");
	close(client_fd);
	close(socket_fd);

	return 0;
}

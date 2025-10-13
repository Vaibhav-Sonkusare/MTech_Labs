#include "../../include/network_utils_v2.h"

struct data_transmited {
    char data1;
    int data2;
    double data3;
};

#define FILE_NAME "data.txt"

void print_data_transmited(struct data_transmited data);
int write_to_file(struct data_transmited *data);
int modify_file(char data1, int data2, double data3);
int read_from_file(struct data_transmited *data);

int main () {
    tcp_server = initialize_tcp_server(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    struct device *client1 = accept_client(tcp_server);
    struct device *client2 = accept_client(tcp_server);

    // receive data
    struct data_transmited data, data_2;
    uint16_t type;
    // receive_custom_struct(client1, &type, &data, sizeof(data));
    recv(client1->fd, &data, sizeof(data), 0);
    printf("Received Data:\n");
    print_data_transmited(data);
    write_to_file(&data);

    // change data
    modify_file('a', 10, 3.14);

    // send modified data
    read_from_file(&data_2);
    printf("Data transmited:\n");
    print_data_transmited(data_2);
    // message_device_custom_struct(client2, MESSAGE_TYPE_CUSTOM_STRUCT, &data, sizeof(data));
    send(client2->fd, &data_2, sizeof(data_2), 0);
    
    // Wait for ack
    char ack_buff[32];
    recv(client2->fd, ack_buff, 32, 0);
    ack_buff[31] = '\0';
    printf("Received ack: %s", ack_buff);

    cleanup_server(tcp_server);
    
    return 0;
}

int write_to_file(struct data_transmited *data) {
	FILE *f = fopen(FILE_NAME, "w");
    if (f == NULL) {
        perror("write_to_file: fopen");
        return -1 * errno;
    }

    fprintf(f, "%c\n", data->data1);
    fprintf(f, "%d\n", data->data2);
    fprintf(f, "%f\n", data->data3);

    fclose(f);

    return 0;
}

int modify_file(char data1, int data2, double data3) {
	FILE *f = fopen(FILE_NAME, "w");
    if (f == NULL) {
        perror("write_to_file: fopen");
        return -1 * errno;
    }

    fprintf(f, "%c\n", data1);
    fprintf(f, "%d\n", data2);
    fprintf(f, "%f\n", data3);

    fclose(f);

    return 0;
}

int read_from_file(struct data_transmited *data) {
    FILE *f = fopen(FILE_NAME, "r");
    if (f == NULL) {
    	perror("read_from_file: fopen");
    	return -1 * errno;
    }
    
    char buffer[32];
    
    memset(buffer, '\0', 32);
    fgets(buffer, 32, f);
    sscanf(buffer, "%c", &(data->data1));
    
    memset(buffer, '\0', 32);
    fgets(buffer, 32, f);
    sscanf(buffer, "%d", &(data->data2));
    
    memset(buffer, '\0', 32);
    fgets(buffer, 32, f);
    sscanf(buffer, "%lf", &(data->data3));
    
    return 0;
}

void print_data_transmited(struct data_transmited data) {
    printf("\tdata->data1 (char) : %c\n", data.data1);
    printf("\tdata->data2 (int)  : %d\n", data.data2);
    printf("\tdata->data3 (float): %f\n", data.data3);
}

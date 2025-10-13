#include "../../include/network_utils_v2.h"

struct data_transmited {
    char data1;
    int data2;
    double data3;
};

void print_data_transmited(struct data_transmited data);

int main () {
    tcp_client = initialize_tcp_client(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    struct data_transmited data;
    uint16_t type;
    // receive_custom_struct(tcp_client, &type, &data, sizeof(data));
    recv(tcp_client->fd, &data, sizeof(data), 0);

    printf("Received Data:\n");
    print_data_transmited(data);

    cleanup_client(tcp_client);
}

void print_data_transmited(struct data_transmited data) {
    printf("\tdata->data1 (char) : %c\n", data.data1);
    printf("\tdata->data2 (int)  : %d\n", data.data2);
    printf("\tdata->data3 (float): %f\n", data.data3);
}
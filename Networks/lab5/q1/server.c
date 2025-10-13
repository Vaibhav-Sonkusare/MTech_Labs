#include "../../include/network_utils_v2.h"

struct data_transmited {
    char data1;
    int data2;
    double data3;
};

void print_data_transmited(struct data_transmited data);

int main () {
    tcp_server = initialize_tcp_server(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    struct device *client1 = accept_client(tcp_server);
    struct device *client2 = accept_client(tcp_server);

    // receive data
    struct data_transmited data;
    uint16_t type;
    // receive_custom_struct(client1, &type, &data, sizeof(data));
    recv(client1->fd, &data, sizeof(data), 0);
    printf("Received Data:\n");
    print_data_transmited(data);

    // change data
    data.data1 = 'a';
    data.data2 = 10;
    data.data3 = 3.14;

    // send modified data
    printf("Data transmited:\n");
    print_data_transmited(data);
    // message_device_custom_struct(client2, MESSAGE_TYPE_CUSTOM_STRUCT, &data, sizeof(data));
    send(client2->fd, &data, sizeof(data), 0);

    cleanup_server(tcp_server);
    
    return 0;
}

void print_data_transmited(struct data_transmited data) {
    printf("\tdata->data1 (char) : %c\n", data.data1);
    printf("\tdata->data2 (int)  : %d\n", data.data2);
    printf("\tdata->data3 (float): %f\n", data.data3);
}
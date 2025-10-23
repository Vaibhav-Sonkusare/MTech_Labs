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
    data.data1 = 'c';
    data.data2 = 3;
    data.data3 = 11.34;

    printf("Data transmited:\n");
    print_data_transmited(data);
    // message_device_custom_struct(tcp_client, MESSAGE_TYPE_CUSTOM_STRUCT, &data, sizeof(data));
    send(tcp_client->fd, &data, sizeof(data), 0);

    cleanup_client(tcp_client);
}

void print_data_transmited(struct data_transmited data) {
    printf("\tdata->data1 (char) : %c\n", data.data1);
    printf("\tdata->data2 (int)  : %d\n", data.data2);
    printf("\tdata->data3 (float): %f\n", data.data3);
}
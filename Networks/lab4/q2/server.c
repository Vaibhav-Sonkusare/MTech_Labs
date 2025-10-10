#include "../../include/network_utils.h"

int main() {
    tcp_server = initialize_tcp_server(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    struct device *cli = accept_client(tcp_server);

    message_client(cli, "Hi port scanner!!!");

    sleep(15);

    cleanup_client(cli);
    cleanup_server(tcp_server);

    return 0;
}
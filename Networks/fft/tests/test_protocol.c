#include <stdio.h>
#include "../include/protocol.h"

int main() {
    pkt_blast_packet_t pkt;
    printf("Size of pkt_blast_packet_t: %zu bytes\n", sizeof(pkt));

    return 0;
}

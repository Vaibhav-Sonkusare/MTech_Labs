#include <stdio.h>
#include "../include/protocol.h"

int main() {
    pkt_file_hdr_t hdr;
    build_pkt_file_hdr(&hdr, "hello.txt", 9, 1234);

    printf("Type: %d\n", hdr.type);
    printf("Rec size: %u\n", hdr._max_rec_size);
    printf("Filename: %s\n", hdr.filename);
    printf("Filesize: %lu\n", hdr.file_size);

    return 0;
}

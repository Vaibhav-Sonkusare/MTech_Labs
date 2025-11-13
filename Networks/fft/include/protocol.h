// protocol.h

#include <stdio.h>

#define MAX_FILENAME_LEN 255
#define MAX_RECORDS_PER_PACKET 16

typedef enum {
    PKT_FILE_HDR = 1,
    PKT_FILE_HDR_ACK,
    PKT_BLAST_HDR,
    PKT_BLAST_DATA,
    PKT_IS_BLAST_OVER,
    PKT_REC_MISS_HDR,
    PKT_REC_MISS_DATA,
    PKT_DISCONNECT
} packet_type_t;

#pragma pack(push, 1)

typedef struct {
    
} pkt_file_hdr_t;

typedef struct {
} pkt_file_hdr_ack_t;

typedef struct {
} pkt_blast_hdr_t;

typedef struct {
} pkt_blast_data_t;

typedef struct {
} pkt_is_blast_over_t;

typedef struct {
} pkt_rec_miss_hdr_t;

typedef struct {
} pkt_rec_miss_data_t;

typedef struct {
} pkt_disconnect_t;
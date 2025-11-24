// protocol.h

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAX_FILENAME_LEN 255                // Maximum filename length
#define DEFAULT_RECORD_SIZE 512             // 512 Bytes
#define DEFAULT_RECORDS_PER_PACKET 16       // Number of records per packet
#define DEFAULT_PACKETS_PER_BLAST 16        // Number of packets per blast

typedef enum {
    PKT_FILE_HDR = 1,
    PKT_FILE_HDR_ACK,
    PKT_BLAST_PACKET,
    PKT_IS_BLAST_OVER,
    PKT_REC_MISS_HDR,
    PKT_DISCONNECT
} packet_type_t;

#pragma pack(push, 1)

typedef struct {
    uint8_t type;

    uint16_t _max_rec_size;
    uint16_t _max_records_per_packet;
    uint16_t _max_packets_per_blast;

    uint64_t file_size;
    uint16_t filename_len;
    char filename[MAX_FILENAME_LEN + 1];
} pkt_file_hdr_t;

typedef struct {
    uint8_t type;

    // 0 = Ok
    // 1 = change in some sizes
    uint8_t status;

    // if status != 0, then modify below values
    uint16_t _max_rec_size;
    uint16_t _max_records_per_packet;
    uint16_t _max_packets_per_blast;
} pkt_file_hdr_ack_t;

typedef struct {
    uint8_t type;

    uint32_t blast_id;
    uint32_t n_packets;

    uint32_t packet_id;
    uint32_t n_records;
    uint32_t record_id[DEFAULT_RECORDS_PER_PACKET];
    
    char data[DEFAULT_RECORDS_PER_PACKET][DEFAULT_RECORD_SIZE];
} pkt_blast_packet_t;

typedef struct {
    uint8_t type;

    uint32_t blast_id;
    uint32_t n_packets;
} pkt_is_blast_over_t;

typedef struct {
    uint8_t type;

    uint8_t n_packets_missing;
    uint8_t is_pkt_missing[DEFAULT_PACKETS_PER_BLAST];
} pkt_rec_miss_hdr_t;

typedef struct {
    uint8_t type;
    uint32_t reason_code;
} pkt_disconnect_t;

#pragma pack(pop)

/*--------------------------Helper Variables Declarations--------------------------*/

// extern const uint16_t MAX_RECORD_SIZE;
// extern const uint16_t MAX_RECORDS_PER_PACKET;
// extern const uint16_t MAX_PACKETS_PER_BLAST;

/*--------------------------Helper Function Declarations--------------------------*/

void build_pkt_file_hdr(pkt_file_hdr_t* pkt, const char* filename, uint16_t file_name_len, uint64_t file_size);
void build_pkt_file_hdr_ack(pkt_file_hdr_ack_t* pkt, uint8_t status, uint16_t max_rec_size, uint16_t max_records_per_packet, uint16_t max_packets_per_blast);
void build_pkt_disconnect(pkt_disconnect_t* pkt, uint32_t reason_code);

#endif  // PROTOCOL_H
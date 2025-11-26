// protocol.c

#include <string.h>
#include <stdint.h>
#include "../include/protocol.h"

/*-----------------------------------------------------------
  FILE_HDR Packet Builder
-----------------------------------------------------------*/
void build_pkt_file_hdr(pkt_file_hdr_t* pkt,
                        const char* filename,
                        uint16_t file_name_len,
                        uint64_t file_size)
{
    if (!pkt) return;

    memset(pkt, 0, sizeof(pkt_file_hdr_t));

    // Packet type
    pkt->type = PKT_FILE_HDR;

    // Sender's maximum capabilities (defaults)
    pkt->_max_rec_size           = DEFAULT_RECORD_SIZE;
    pkt->_max_records_per_packet = DEFAULT_RECORDS_PER_PACKET;
    pkt->_max_packets_per_blast  = DEFAULT_PACKETS_PER_BLAST;

    // File metadata
    pkt->file_size   = file_size;
    pkt->filename_len = file_name_len;

    // Copy filename safely
    if (file_name_len > MAX_FILENAME_LEN)
        file_name_len = MAX_FILENAME_LEN;

    memset(pkt->filename, '\0', MAX_FILENAME_LEN + 1);
    memcpy(pkt->filename, filename, file_name_len);
    pkt->filename[file_name_len] = '\0';   // Null-terminate
}

/*-----------------------------------------------------------
  FILE_HDR_ACK Packet Builder
-----------------------------------------------------------*/
void build_pkt_file_hdr_ack(pkt_file_hdr_ack_t* pkt,
                            uint8_t status,
                            uint16_t max_rec_size,
                            uint16_t max_records_per_packet,
                            uint16_t max_packets_per_blast)
{
    if (!pkt) return;

    memset(pkt, 0, sizeof(pkt_file_hdr_ack_t));

    pkt->type = PKT_FILE_HDR_ACK;

    pkt->status = status;

    // These values only matter if status != 0
    pkt->_max_rec_size           = max_rec_size;
    pkt->_max_records_per_packet = max_records_per_packet;
    pkt->_max_packets_per_blast  = max_packets_per_blast;
}

/*-----------------------------------------------------------
  IS_BLAST_OVER Packet Builder
-----------------------------------------------------------*/
void build_pkt_is_blast_over(pkt_is_blast_over_t *pkt,
                             uint32_t blast_id,
                             uint32_t n_packets)
{
    memset(pkt, 0, sizeof(pkt_is_blast_over_t));
    pkt->type = PKT_IS_BLAST_OVER;
    pkt->blast_id = blast_id;
    pkt->n_packets = n_packets;
}

/*-----------------------------------------------------------
  REC_MISS_HDR Packet Builder
-----------------------------------------------------------*/
void build_pkt_rec_miss_hdr(pkt_rec_miss_hdr_t *pkt,
                            uint8_t n_missing,
                            const uint8_t *missing_arr)
{
    memset(pkt, 0, sizeof(pkt_rec_miss_hdr_t));
    pkt->type = PKT_REC_MISS_HDR;
    pkt->n_packets_missing = n_missing;
    memcpy(pkt->is_pkt_missing,
           missing_arr,
           DEFAULT_PACKETS_PER_BLAST);
}



/*-----------------------------------------------------------
  DISCONNECT Packet Builder
-----------------------------------------------------------*/
void build_pkt_disconnect(pkt_disconnect_t* pkt, uint32_t reason_code)
{
    if (!pkt) return;

    memset(pkt, 0, sizeof(pkt_disconnect_t));

    pkt->type = PKT_DISCONNECT;
    pkt->reason_code = reason_code;
}

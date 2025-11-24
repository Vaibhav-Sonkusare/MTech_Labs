#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

/* Return value: number of bytes written, or 0 on error (buffer too small). */
/* FILE HDR */
size_t serialize_file_hdr(uint8_t *buf, size_t buf_len, const pkt_file_hdr_t *pkt);
size_t deserialize_file_hdr(const uint8_t *buf, size_t buf_len, pkt_file_hdr_t *out);

/* FILE_HDR_ACK */
size_t serialize_file_hdr_ack(uint8_t *buf, size_t buf_len, const pkt_file_hdr_ack_t *pkt);
size_t deserialize_file_hdr_ack(const uint8_t *buf, size_t buf_len, pkt_file_hdr_ack_t *out);

/* BLAST / DATA packet
   - record_ids: array of n_records uint32_t (host order)
   - records_blob: pointer to concatenated records (n_records * rec_size bytes)
   - rec_size: size in bytes of each record (agreed by protocol)
*/
size_t serialize_blast_packet(uint8_t *buf, size_t buf_len,
                              uint32_t blast_id, uint32_t packet_id,
                              uint32_t n_packets, /* optional: keep for compat */
                              uint16_t n_records,
                              const uint32_t *record_ids,
                              const uint8_t *records_blob,
                              uint16_t rec_size);

size_t deserialize_blast_packet(const uint8_t *buf, size_t buf_len,
                                uint32_t *out_blast_id, uint32_t *out_packet_id,
                                uint16_t *out_n_records,
                                uint32_t *out_record_ids, /* caller-provided array, capacity >= DEFAULT_RECORDS_PER_PACKET */
                                uint8_t *out_records_blob, /* caller-provided buffer capacity >= DEFAULT_RECORDS_PER_PACKET * rec_size */
                                uint16_t rec_size);

/* IS_BLAST_OVER */
size_t serialize_is_blast_over(uint8_t *buf, size_t buf_len, const pkt_is_blast_over_t *pkt);
size_t deserialize_is_blast_over(const uint8_t *buf, size_t buf_len, pkt_is_blast_over_t *out);

/* REC_MISS_HDR (packet-mask style per your current struct) */
size_t serialize_rec_miss_hdr(uint8_t *buf, size_t buf_len, const pkt_rec_miss_hdr_t *pkt);
size_t deserialize_rec_miss_hdr(const uint8_t *buf, size_t buf_len, pkt_rec_miss_hdr_t *out);

/* DISCONNECT */
size_t serialize_disconnect(uint8_t *buf, size_t buf_len, const pkt_disconnect_t *pkt);
size_t deserialize_disconnect(const uint8_t *buf, size_t buf_len, pkt_disconnect_t *out);

#endif /* SERIALIZE_H */

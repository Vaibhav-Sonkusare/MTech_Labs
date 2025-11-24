#include <string.h>
#include <arpa/inet.h>
#include "serialize.h"
#include "protocol.h"

/* Helper for 64-bit network order */
static uint64_t htonll(uint64_t x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return (((uint64_t)htonl((uint32_t)(x & 0xFFFFFFFFULL))) << 32) |
            htonl((uint32_t)(x >> 32));
#else
    return x;
#endif
}
static uint64_t ntohll(uint64_t x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return (((uint64_t)ntohl((uint32_t)(x & 0xFFFFFFFFULL))) << 32) |
            ntohl((uint32_t)(x >> 32));
#else
    return x;
#endif
}

/* ---------------- FILE HDR ---------------- */
size_t serialize_file_hdr(uint8_t *buf, size_t buf_len, const pkt_file_hdr_t *pkt) {
    if (!buf || !pkt) return 0;

    /* required length = 1 + 2+2+2 +8 +4 +2 + filename_len
       but your pkt_file_hdr_t layout uses:
       type(1) + _max_rec_size(2) + _max_records_per_packet(2) + _max_packets_per_blast(2)
       + file_size(8) + filename_len(2) + filename bytes
    */
    size_t need = 1 + 2 + 2 + 2 + 8 + 2 + pkt->filename_len;
    if (buf_len < need) return 0;

    size_t off = 0;
    buf[off++] = pkt->type;

    uint16_t tmp16;
    tmp16 = htons(pkt->_max_rec_size);
    memcpy(buf + off, &tmp16, 2); off += 2;

    tmp16 = htons(pkt->_max_records_per_packet);
    memcpy(buf + off, &tmp16, 2); off += 2;

    tmp16 = htons(pkt->_max_packets_per_blast);
    memcpy(buf + off, &tmp16, 2); off += 2;

    uint64_t tmp64 = htonll(pkt->file_size);
    memcpy(buf + off, &tmp64, 8); off += 8;

    uint16_t fname_len = htons(pkt->filename_len);
    memcpy(buf + off, &fname_len, 2); off += 2;

    if (pkt->filename_len > 0) {
        memcpy(buf + off, pkt->filename, pkt->filename_len);
        off += pkt->filename_len;
    }
    return off;
}

size_t deserialize_file_hdr(const uint8_t *buf, size_t buf_len, pkt_file_hdr_t *out) {
    if (!buf || !out) return 0;
    if (buf_len < 1 + 2+2+2 +8 +2) return 0;

    size_t off = 0;
    out->type = buf[off++];

    uint16_t tmp16;
    memcpy(&tmp16, buf + off, 2); off += 2;
    out->_max_rec_size = ntohs(tmp16);

    memcpy(&tmp16, buf + off, 2); off += 2;
    out->_max_records_per_packet = ntohs(tmp16);

    memcpy(&tmp16, buf + off, 2); off += 2;
    out->_max_packets_per_blast = ntohs(tmp16);

    uint64_t tmp64;
    memcpy(&tmp64, buf + off, 8); off += 8;
    out->file_size = ntohll(tmp64);

    uint16_t fname_len;
    memcpy(&fname_len, buf + off, 2); off += 2;
    fname_len = ntohs(fname_len);

    if (fname_len > MAX_FILENAME_LEN) return 0;
    if (buf_len < off + fname_len) return 0;

    out->filename_len = fname_len;
    if (fname_len > 0) {
        memcpy(out->filename, buf + off, fname_len);
    }
    out->filename[fname_len] = '\0';
    off += fname_len;

    return off;
}

/* ---------------- FILE_HDR_ACK ---------------- */
size_t serialize_file_hdr_ack(uint8_t *buf, size_t buf_len, const pkt_file_hdr_ack_t *pkt) {
    if (!buf || !pkt) return 0;
    size_t need = 1 + 1 + 2 + 2; /* type + status + _max_rec_size + _max_records_per_packet (we include _max_packets too) */
    need = 1 + 1 + 2 + 2 + 2;
    if (buf_len < need) return 0;
    size_t off = 0;
    buf[off++] = pkt->type;
    buf[off++] = pkt->status;

    uint16_t tmp16;
    tmp16 = htons(pkt->_max_rec_size);
    memcpy(buf + off, &tmp16, 2); off += 2;
    tmp16 = htons(pkt->_max_records_per_packet);
    memcpy(buf + off, &tmp16, 2); off += 2;
    tmp16 = htons(pkt->_max_packets_per_blast);
    memcpy(buf + off, &tmp16, 2); off += 2;
    return off;
}

size_t deserialize_file_hdr_ack(const uint8_t *buf, size_t buf_len, pkt_file_hdr_ack_t *out) {
    if (!buf || !out) return 0;
    if (buf_len < 1 + 1 + 2 + 2 + 2) return 0;
    size_t off = 0;
    out->type = buf[off++];
    out->status = buf[off++];
    uint16_t tmp16;
    memcpy(&tmp16, buf + off, 2); off += 2; out->_max_rec_size = ntohs(tmp16);
    memcpy(&tmp16, buf + off, 2); off += 2; out->_max_records_per_packet = ntohs(tmp16);
    memcpy(&tmp16, buf + off, 2); off += 2; out->_max_packets_per_blast = ntohs(tmp16);
    return off;
}

/* ---------------- BLAST / DATA packet ----------------
   Layout (serialized):
   [1] type
   [4] blast_id
   [4] n_packets (optional/kept for compat)
   [4] packet_id
   [2] n_records
   For i in 0..n_records-1:
     [4] record_id
     [rec_size] record_data
*/
size_t serialize_blast_packet(uint8_t *buf, size_t buf_len,
                              uint32_t blast_id, uint32_t packet_id,
                              uint32_t n_packets,
                              uint16_t n_records,
                              const uint32_t *record_ids,
                              const uint8_t *records_blob,
                              uint16_t rec_size)
{
    if (!buf || !record_ids || (n_records > 0 && !records_blob)) return 0;

    size_t off = 0;
    /* base header size */
    size_t need = 1 + 4 + 4 + 4 + 2 + /* per-record: 4 + rec_size */
                  (size_t)n_records * (4 + rec_size);
    if (buf_len < need) return 0;

    buf[off++] = PKT_BLAST_PACKET;

    uint32_t tmp32;
    tmp32 = htonl(blast_id);
    memcpy(buf + off, &tmp32, 4); off += 4;

    tmp32 = htonl(n_packets);
    memcpy(buf + off, &tmp32, 4); off += 4;

    tmp32 = htonl(packet_id);
    memcpy(buf + off, &tmp32, 4); off += 4;

    uint16_t tmp16 = htons(n_records);
    memcpy(buf + off, &tmp16, 2); off += 2;

    /* write records */
    for (uint16_t i = 0; i < n_records; ++i) {
        uint32_t rid = htonl(record_ids[i]);
        memcpy(buf + off, &rid, 4); off += 4;

        memcpy(buf + off, records_blob + ((size_t)i * rec_size), rec_size);
        off += rec_size;
    }

    return off;
}

size_t deserialize_blast_packet(const uint8_t *buf, size_t buf_len,
                                uint32_t *out_blast_id, uint32_t *out_packet_id,
                                uint16_t *out_n_records,
                                uint32_t *out_record_ids,
                                uint8_t *out_records_blob,
                                uint16_t rec_size)
{
    if (!buf || !out_blast_id || !out_packet_id || !out_n_records) return 0;
    if (buf_len < 1 + 4 + 4 + 4 + 2) return 0;
    size_t off = 0;
    uint8_t type = buf[off++];
    if (type != PKT_BLAST_PACKET) return 0;

    uint32_t tmp32;
    memcpy(&tmp32, buf + off, 4); off += 4;
    *out_blast_id = ntohl(tmp32);

    /* n_packets (ignored, but consume) */
    memcpy(&tmp32, buf + off, 4); off += 4;
    (void)ntohl(tmp32);

    memcpy(&tmp32, buf + off, 4); off += 4;
    *out_packet_id = ntohl(tmp32);

    uint16_t tmp16;
    memcpy(&tmp16, buf + off, 2); off += 2;
    uint16_t n_records = ntohs(tmp16);

    if (n_records > DEFAULT_RECORDS_PER_PACKET) return 0;

    /* check available length */
    size_t need = (size_t)off + (size_t)n_records * (4 + rec_size);
    if (buf_len < need) return 0;

    for (uint16_t i = 0; i < n_records; ++i) {
        memcpy(&tmp32, buf + off, 4); off += 4;
        out_record_ids[i] = ntohl(tmp32);

        memcpy(out_records_blob + ((size_t)i * rec_size), buf + off, rec_size);
        off += rec_size;
    }

    *out_n_records = n_records;
    return off;
}

/* ---------------- IS_BLAST_OVER ---------------- */
size_t serialize_is_blast_over(uint8_t *buf, size_t buf_len, const pkt_is_blast_over_t *pkt) {
    if (!buf || !pkt) return 0;
    size_t need = 1 + 4 + 4 + 4;
    if (buf_len < need) return 0;
    size_t off = 0;
    buf[off++] = pkt->type;
    uint32_t tmp32 = htonl(pkt->blast_id);
    memcpy(buf + off, &tmp32, 4); off += 4;
    tmp32 = htonl(pkt->blast_id); /* if you intended st_record/fin_record fields, adjust */
    memcpy(buf + off, &tmp32, 4); off += 4;
    tmp32 = htonl(pkt->blast_id); /* placeholder if different; keep for compat */
    memcpy(buf + off, &tmp32, 4); off += 4;
    return off;
}
size_t deserialize_is_blast_over(const uint8_t *buf, size_t buf_len, pkt_is_blast_over_t *out) {
    if (!buf || !out) return 0;
    if (buf_len < 1 + 4 + 4 + 4) return 0;
    size_t off = 0;
    out->type = buf[off++];
    uint32_t tmp32;
    memcpy(&tmp32, buf + off, 4); off += 4; out->blast_id = ntohl(tmp32);
    /* If you had st_record/fin_record fields, parse them here. For current pkt_is_blast_over_t, only blast_id+n_packets are present. */
    memcpy(&tmp32, buf + off, 4); off += 4; (void)ntohl(tmp32);
    memcpy(&tmp32, buf + off, 4); off += 4; (void)ntohl(tmp32);
    return off;
}

/* ---------------- REC_MISS_HDR (packet mask style) ---------------- */
size_t serialize_rec_miss_hdr(uint8_t *buf, size_t buf_len, const pkt_rec_miss_hdr_t *pkt) {
    if (!buf || !pkt) return 0;
    size_t need = 1 + 1 + DEFAULT_PACKETS_PER_BLAST;
    if (buf_len < need) return 0;
    size_t off = 0;
    buf[off++] = pkt->type;
    buf[off++] = pkt->n_packets_missing;
    memcpy(buf + off, pkt->is_pkt_missing, DEFAULT_PACKETS_PER_BLAST);
    off += DEFAULT_PACKETS_PER_BLAST;
    return off;
}
size_t deserialize_rec_miss_hdr(const uint8_t *buf, size_t buf_len, pkt_rec_miss_hdr_t *out) {
    if (!buf || !out) return 0;
    size_t need = 1 + 1 + DEFAULT_PACKETS_PER_BLAST;
    if (buf_len < need) return 0;
    size_t off = 0;
    out->type = buf[off++];
    out->n_packets_missing = buf[off++];
    memcpy(out->is_pkt_missing, buf + off, DEFAULT_PACKETS_PER_BLAST);
    off += DEFAULT_PACKETS_PER_BLAST;
    return off;
}

/* ---------------- DISCONNECT ---------------- */
size_t serialize_disconnect(uint8_t *buf, size_t buf_len, const pkt_disconnect_t *pkt) {
    if (!buf || !pkt) return 0;
    size_t need = 1 + 4;
    if (buf_len < need) return 0;
    size_t off = 0;
    buf[off++] = pkt->type;
    uint32_t tmp32 = htonl(pkt->reason_code);
    memcpy(buf + off, &tmp32, 4); off += 4;
    return off;
}
size_t deserialize_disconnect(const uint8_t *buf, size_t buf_len, pkt_disconnect_t *out) {
    if (!buf || !out) return 0;
    size_t need = 1 + 4;
    if (buf_len < need) return 0;
    size_t off = 0;
    out->type = buf[off++];
    uint32_t tmp32;
    memcpy(&tmp32, buf + off, 4); off += 4;
    out->reason_code = ntohl(tmp32);
    return off;
}

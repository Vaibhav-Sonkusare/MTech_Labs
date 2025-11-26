#ifndef BLAST_H
#define BLAST_H

#include <stdint.h>
#include "../include/protocol.h"
#include "../include/record.h"

/*
 * build_blast_packet:
 *   Builds ONE pkt_blast_packet_t containing up to max_per_packet records.
 *
 * Parameters:
 *   pkt  → output packet (struct already allocated by caller)
 *   blast_id → which blast this packet belongs to
 *   packet_id → index of packet inside this blast (0-based or 1-based; you decide)
 *   records → array of all records
 *   start_idx → global index into records[] to pack from
 *   nrec_total → total number of records in entire file
 *   max_per_packet → limit negotiated in protocol (DEFAULT_RECORDS_PER_PACKET)
 *
 * Returns:
 *   number of records packed into this packet (≤ max_per_packet)
 */
uint16_t build_blast_packet(
        pkt_blast_packet_t *pkt,
        uint32_t blast_id,
        uint32_t packet_id,
        const record_t *records,
        uint32_t start_idx,
        uint32_t nrec_total,
        uint16_t max_per_packet);

#endif

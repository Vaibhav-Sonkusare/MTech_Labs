// blast.c

#include "../include/blast.h"
#include <string.h>

/*
 * Builds ONE blast packet containing:
 *   { record_id[i], record_data[i] } repeated N times
 *
 * No dynamic allocation.
 */
uint16_t build_blast_packet(pkt_blast_packet_t *pkt,
                            uint32_t blast_id,
                            uint32_t packet_id,
                            const record_t *records,
                            uint32_t start_idx,
                            uint32_t nrec_total,
                            uint16_t max_records_per_packet)
{
    if (!pkt || !records) return 0;

    memset(pkt, 0, sizeof(pkt_blast_packet_t));

    pkt->type = PKT_BLAST_PACKET;
    pkt->blast_id = blast_id;

    // n_packets is not known here; caller will set it later.
    // For safety, set temporary placeholder:
    pkt->n_packets = 0;

    pkt->packet_id = packet_id;
    pkt->n_records = 0;

    uint16_t count = 0;

    for (uint16_t i = 0; i < max_records_per_packet; ++i) {
        uint32_t idx = start_idx + i;
        if (idx >= nrec_total) break;

        pkt->record_id[count] = records[idx].record_id;

        // Copy actual record data (fixed-size records)
        memset(pkt->data[count], '\0', DEFAULT_RECORD_SIZE);
        memcpy(pkt->data[count], records[idx].data, records[idx].size);

        count++;
    }

    pkt->n_records = count;
    return count;
}

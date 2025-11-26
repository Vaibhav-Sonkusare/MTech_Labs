// record.h

#ifndef RECORD_H
#define RECORD_H

#include <stdint.h>
#include <stdlib.h>

/* record_t: internal representation of one record.
   Note: `data` points into a contiguous data blob allocated by
   read_file_to_records() for efficiency. Do NOT free data individually.
*/
typedef struct {
    uint32_t record_id;   /* 1-based global record number */
    uint8_t  *data;       /* pointer to rec_size bytes */
    uint16_t size;        /* actual record size (== rec_size except maybe last) */
} record_t;

#endif /* RECORD_H */

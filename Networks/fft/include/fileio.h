// fileio.h

#ifndef FILEIO_H
#define FILEIO_H

#include <stdint.h>
#include "../include/record.h"

int read_file_to_records(const char *path, uint16_t rec_size,
                         record_t **out_recs, uint32_t *out_nrec, uint64_t *out_file_size);
void free_records(record_t *recs, uint32_t nrec);

#endif

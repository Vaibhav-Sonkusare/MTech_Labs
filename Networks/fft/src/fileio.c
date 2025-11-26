// fileio.c

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/fileio.h"
#include "../include/record.h"

/*
 * read_file_to_records:
 *  - path: input file path
 *  - rec_size: record size in bytes (e.g., DEFAULT_RECORD_SIZE)
 *  - out_recs: pointer to an allocated array of record_t (caller must free via free_records)
 *  - out_nrec: number of records created
 *
 * Returns 0 on success, -1 on error.
 *
 * Implementation notes:
 *  - Allocates one contiguous buffer: data_blob = nrec * rec_size bytes.
 *  - Allocates an array of record_t of length nrec.
 *  - For the last record, if file size isn't a multiple of rec_size, the tail is zero-padded.
 */
int read_file_to_records(const char *path, uint16_t rec_size,
                         record_t **out_recs, uint32_t *out_nrec, uint64_t *out_file_size)
{
    if (!path || !out_recs || !out_nrec) return -1;
    if (rec_size == 0) return -1;

    struct stat st;
    if (stat(path, &st) != 0) {
        perror("stat");
        return -1;
    }
    uint64_t file_size = (uint64_t)st.st_size;
    *out_file_size = file_size;

    uint32_t nrec = (file_size + rec_size - 1) / rec_size;
    if (nrec == 0) nrec = 1; /* handle empty file: one zeroed record */

    /* allocate one big blob */
    uint8_t *blob = calloc((size_t)nrec, rec_size);
    if (!blob) {
        perror("calloc");
        return -1;
    }

    /* open and read */
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        free(blob);
        return -1;
    }

    size_t to_read = (size_t)file_size;
    size_t off = 0;
    while (to_read > 0) {
        size_t r = fread(blob + off, 1, to_read, f);
        if (r == 0) {
            if (feof(f)) break;
            if (ferror(f)) {
                perror("fread");
                fclose(f);
                free(blob);
                return -1;
            }
        }
        off += r;
        to_read -= r;
    }
    fclose(f);

    /* allocate record_t array */
    record_t *recs = malloc(sizeof(record_t) * nrec);
    if (!recs) {
        perror("malloc");
        free(blob);
        return -1;
    }

    for (uint32_t i = 0; i < nrec; ++i) {
        recs[i].record_id = i + 1;
        recs[i].data = blob + (size_t)i * rec_size;

        if (i < nrec - 1) {
            recs[i].size = rec_size;
        } else {
            // last record
            uint64_t offset = (uint64_t)i * rec_size;
            uint64_t remaining = file_size - offset;
            recs[i].size = (uint32_t)remaining;
        }
    }

    *out_recs = recs;
    *out_nrec = nrec;
    return 0;
}

/* free_records: frees the record_t array and its underlying blob */
void free_records(record_t *recs, uint32_t nrec)
{
    if (!recs) return;
    if (nrec == 0) {
        free(recs);
        return;
    }
    /* the data blob pointer is recs[0].data if allocated by read_file_to_records */
    uint8_t *blob = recs[0].data;
    free(blob);
    free(recs);
}

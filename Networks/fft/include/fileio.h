// fileio.h

#ifndef FILEIO_H
#define FILEIO_H

#include "../include/record.h"
#include <stdint.h>

#include <stdio.h>

/* Opens the file for reading. Returns NULL on error. */
FILE *file_open_read(const char *path, uint64_t *out_file_size);

/*
 * Reads up to 'size' bytes into 'buf'.
 * Returns number of bytes read, or -1 on error.
 * Handles zero-padding if less bytes are available and we want full chunks?
 * Actually, caller should handle padding logic if needed, but for record
 * splitting we can just read what's there and let the sender fill zeros.
 */
int file_read_chunk(FILE *f, uint8_t *buf, int size);

#endif

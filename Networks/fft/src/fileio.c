// fileio.c

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../include/fileio.h"

/*
 * file_open_read: wrappers fopen and stat
 */
FILE *file_open_read(const char *path, uint64_t *out_file_size) {
  if (!path || !out_file_size)
    return NULL;

  struct stat st;
  if (stat(path, &st) != 0) {
    perror("stat");
    return NULL;
  }
  *out_file_size = (uint64_t)st.st_size;

  FILE *f = fopen(path, "rb");
  if (!f) {
    perror("fopen");
    return NULL;
  }
  return f;
}

/*
 * file_read_chunk: similar to fread but ensures we try to read exactly size
 * bytes Returns bytes read. If error, returns -1.
 */
int file_read_chunk(FILE *f, uint8_t *buf, int size) {
  if (!f || !buf || size <= 0)
    return -1;

  size_t r = fread(buf, 1, size, f);
  if (r < (size_t)size) {
    if (ferror(f)) {
      perror("fread");
      return -1;
    }
    // Short read (EOF) is fine, just return what we got
  }
  return (int)r;
}

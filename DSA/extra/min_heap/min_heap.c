// min_heap.c

#include "min_heap.h"
#include <stdio.h>

struct min_heap {
    int *data;
    int size;
    int capacity;
};
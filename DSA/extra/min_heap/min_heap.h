// min_heap.h

#ifndef MIN_HEAP_H
#define MIN_HEAP_H

typedef struct min_heap Min_Heap;

// Main Function Definition
Min_Heap *min_heap_create(int capacity);
void min_heap_insert(Min_Heap *min_heap, int value);
int *min_heap_peek_min(Min_Heap *min_heap);
int *min_heap_search(Min_Heap *min_heap, int key);
void min_heap_delete();

#endif  // MIN_HEAP_H
#include "kernel.h"
#include "string.h"  

#define HEAP_SIZE 0x200000   // 2 MB

static unsigned char heap[HEAP_SIZE];
static unsigned int heap_ptr = 0;

void *malloc(unsigned int size) {
    if (heap_ptr + size > HEAP_SIZE) return 0;
    void *ptr = heap + heap_ptr;
    heap_ptr += size;
    return ptr;
}

void free(void *ptr) {
    // No-op for bump allocator
}

void *realloc(void *ptr, unsigned int new_size) {
    if (!ptr) return malloc(new_size);
    if (new_size == 0) {
        free(ptr);
        return 0;
    }
    void *new_ptr = malloc(new_size);
    if (new_ptr) {
        // Assume old size <= new_size; copy new_size bytes (may over-read, but TCC enlarges buffers)
        memcpy(new_ptr, ptr, new_size);
    }
    free(ptr);
    return new_ptr;
}

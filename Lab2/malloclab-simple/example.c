#include <stdio.h>
#include "mm.h"

#define malloc(size) mm_malloc(size)
#define free(ptr) mm_free(ptr)

int main() {
    // Initialize the memory manager
    mm_init();

    // Allocate some memory
    void* ptr1 = mm_malloc(16);
    void* ptr2 = mm_malloc(32);
    void* ptr3 = mm_malloc(64);

    // Print the addresses of the allocated blocks
    printf("Allocated blocks:\n");
    printf("ptr1: %p\n", ptr1);
    printf("ptr2: %p\n", ptr2);
    printf("ptr3: %p\n", ptr3);

    // Free the allocated memory
    mm_free(ptr1);
    mm_free(ptr2);
    mm_free(ptr3);

    ptr1 = mm_malloc(16);
    ptr2 = mm_malloc(32);
    ptr3 = mm_malloc(64);

    // Print the addresses of the allocated blocks
    printf("Allocated blocks after freeing:\n");
    printf("ptr1: %p\n", ptr1);
    printf("ptr2: %p\n", ptr2);
    printf("ptr3: %p\n", ptr3);

    // Free the allocated memory again
    mm_free(ptr1);
    mm_free(ptr2);
    mm_free(ptr3);

    return 0;
}

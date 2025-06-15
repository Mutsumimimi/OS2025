/*
 * Memory utilization calculation fix
 * The function should return the ratio of user requested memory to total heap size
 */
 
 double get_utilization() {
    if (heap_size == 0) {
        return 0.0;
    }
    return (double)user_malloc_size / heap_size;
}

/*
 * You should update user_malloc_size and heap_size in the following functions:
 * 
 * 1. In mm_malloc: Increase user_malloc_size by the requested size when allocating
 * 2. In mm_free: Decrease user_malloc_size by the size of the freed block
 * 3. In extend_heap: Increase heap_size by the extended size
 * 
 * Here are the modifications:
 */

// In mm_malloc, add after successful allocation (in place() function):
user_malloc_size += size; // Add the user-requested size

// In mm_free, add at the beginning:
size_t block_size = GET_SIZE(HDRP(bp));
user_malloc_size -= block_size - WSIZE; // Subtract user space (block size minus header)

// In extend_heap, add after successful heap extension:
heap_size += size; // Add the extended heap size

/*
 * Bug fixes for coalesce function
 * The main issues are with how next and previous block sizes are calculated
 */
 
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1: Both blocks are allocated */
        // Current implementation is correct
        PUT(HDRP(bp), PACK(size, prev_alloc, 0));
        PUT(FTRP(bp), PACK(size, prev_alloc, 0));
        add_to_free_list(bp);
    }
    else if (prev_alloc && !next_alloc) { /* Case 2: Next block is free */
        // Fix: Get size directly from next block's header
        void *next_bp = NEXT_BLKP(bp);
        size_t next_size = GET_SIZE(HDRP(next_bp));
        
        delete_from_free_list(next_bp);
        size += next_size;
        PUT(HDRP(bp), PACK(size, prev_alloc, 0));
        PUT(FTRP(bp), PACK(size, prev_alloc, 0));
        add_to_free_list(bp);
    }
    else if (!prev_alloc && next_alloc) { /* Case 3: Previous block is free */
        // Fix: Get prev block properly
        void *prev_bp = PREV_BLKP(bp);
        size_t prev_size = GET_SIZE(HDRP(prev_bp));
        
        delete_from_free_list(prev_bp);
        size += prev_size;
        bp = prev_bp;  // Move bp to start of previous block
        
        PUT(HDRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(prev_bp)), 0));
        PUT(FTRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(prev_bp)), 0));
        add_to_free_list(bp);
    }
    else { /* Case 4: Both prev and next blocks are free */
        void *prev_bp = PREV_BLKP(bp);
        void *next_bp = NEXT_BLKP(bp);
        size_t prev_size = GET_SIZE(HDRP(prev_bp));
        size_t next_size = GET_SIZE(HDRP(next_bp));
        
        delete_from_free_list(prev_bp);
        delete_from_free_list(next_bp);
        
        size += prev_size + next_size;
        bp = prev_bp;  // Move bp to start of previous block
        
        PUT(HDRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(prev_bp)), 0));
        PUT(FTRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(prev_bp)), 0));
        add_to_free_list(bp);
    }
    
    return bp;
}

/*
 * Bug fixes for the find_fit functions
 */

#if FIRST_FIT
static void *find_fit_first(size_t asize) {
    void *bp = free_listp;
    
    if (free_listp == NULL)
        return NULL;
        
    while (bp != NULL) {
        if (GET_SIZE(HDRP(bp)) >= asize)
            return bp;
        bp = (void *)GET_SUCC(bp);
    }
    
    return NULL;
}
#else
static void* find_fit_best(size_t asize) {
    void *bp = free_listp;
    void *best_bp = NULL;
    size_t best_size = 0;  // Initialize to 0 (will be set on first match)
    
    if (free_listp == NULL)
        return NULL;
        
    while (bp != NULL) {
        size_t current_size = GET_SIZE(HDRP(bp));
        if (current_size >= asize) {
            // First match or better match than previous best
            if (best_bp == NULL || current_size < best_size) {
                best_bp = bp;
                best_size = current_size;
            }
        }
        bp = (void *)GET_SUCC(bp);
    }
    
    return best_bp;
}
#endif

/*
 * Bug fixes for the place function
 */
static void place(void *bp, size_t asize) {
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t size = GET_SIZE(HDRP(bp));
    size_t free_size = size - asize;
    
    // Update memory utilization tracking
    user_malloc_size += asize - WSIZE; // Account for user space (minus header)
    
    // Remove this block from free list in any case
    delete_from_free_list(bp);
    
    if (free_size < MIN_BLK_SIZE) {
        // If remaining space is too small for a free block,
        // allocate the entire block
        PUT(HDRP(bp), PACK(size, prev_alloc, 1));
        
        // Update next block's prev_alloc bit to 1
        void *next_bp = NEXT_BLKP(bp);
        if (GET_SIZE(HDRP(next_bp)) != 0) { // Not the epilogue block
            size_t next_size = GET_SIZE(HDRP(next_bp));
            int next_alloc = GET_ALLOC(HDRP(next_bp));
            PUT(HDRP(next_bp), PACK(next_size, 1, next_alloc));
        }
    } 
    else {
        // Split the block
        PUT(HDRP(bp), PACK(asize, prev_alloc, 1));
        
        // Create new free block
        void *free_bp = NEXT_BLKP(bp);
        PUT(HDRP(free_bp), PACK(free_size, 1, 0));  // Set prev_alloc to 1
        PUT(FTRP(free_bp), PACK(free_size, 1, 0));
        
        // Add the new free block to the free list
        add_to_free_list(free_bp);
    }
}
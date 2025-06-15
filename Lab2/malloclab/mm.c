/*
 * mm.c - malloc package with explicit free list and first fit/best fit search
 *
 * NOTE_TO_STUDENTS: Highly recommend you read the experiment tutorial
 * and get a clear view about the memory structure of free and allocated
 * block before you start coding.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


/* Word takes 8 bytes and double word takes 16 bytes */
#define WSIZE 8
#define DSIZE 16
#define CHUNKSIZE (1 << 12)
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/*
    Each head or foot is WSIZE-sized and organized as below:
    |---------size (62bits)----------|--prev_alloc (1 bit)--|---alloc (1 bit)---|

    Value of size is a multiple of 8, so the 2 lowest bits can be used by prev_alloc and alloc.
*/
/* Pack each argument in the order in brackets */
#define PACK(size, prev_alloc, alloc) (((size) & ~0x7) | ((prev_alloc << 1) & ~0x1) | (alloc)) // In fact, we enforce SIZE to be multiple of 8 :)
#define PACK_PREV_ALLOC(val, prev_alloc) ((val & ~(1<<1)) | (prev_alloc << 1))
#define PACK_ALLOC(val, alloc) ((val) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned long *)(p))
#define PUT(p, val) (*(unsigned long *)(p) = (val))

/* Use mask to get different fields at address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) ((GET(p) & 0x2) >> 1)

/* Get head, foot, previous and next block of block bp.
   NOTE: bp is the beginning address of the block, not the addressof head */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) /*only for free blk*/
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE))) /*only when prev_block is free, which can usd*/

#define GET_PRED(bp) (GET(bp))
#define SET_PRED(bp, val) (PUT(bp, val))

#define GET_SUCC(bp) (GET(bp + WSIZE))
#define SET_SUCC(bp, val) (PUT(bp + WSIZE, val))

#define MIN_BLK_SIZE (2 * DSIZE)
/*explicit free list end*/

/* single word (8) or double word (16) alignment */
#define ALIGNMENT WSIZE

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_listp;
static char *free_listp;

#if FIRST_FIT
static void *find_fit_first(size_t asize);
#else
static void *find_fit_best(size_t asize);
#endif

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
// static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void add_to_free_list(void *bp);
static void delete_from_free_list(void *bp);
double get_utilization();
void mm_check(const char * function, char* bp);

/*
    TODO:
        完成一个简单的分配器内存使用率统计
        user_malloc_size: 用户申请内存量
        heap_size: 分配器占用内存量
    HINTS:
        1. 在适当的地方修改上述两个变量，细节参考实验文档
        2. 在 get_utilization() 中计算使用率并返回
*/
size_t user_malloc_size = 0;
size_t heap_size = 0;
double get_utilization() {
    if (heap_size == 0) {
        return 0.0;
    }
    return (double)user_malloc_size / heap_size;
    // return 0;
}
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    mem_init();     // 请添加该行。
    free_listp = NULL;

    // 给 heap_listp 这个地址赋值
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    // 给 heap_listp 地址开头的连续四个内存空间写数据
    PUT(heap_listp, 0);     // 0 表示空闲块，1 表示分配块
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize;         /* Adjusted block size */
    size_t extend_size;     /* Amount to extend head if not fit */
    char *bp;

    /* Ignore spurious requesets */
    if (size == 0)
        return NULL;
    /* Adjust block size to include overhead and alignment reqs. */
    newsize = MAX(MIN_BLK_SIZE, ALIGN((size + WSIZE)));

    /* Search the free list for a fit */
    #if FIRST_FIT
    if ((bp = find_fit_first(newsize)) != NULL)
    {
        // mm_check(__FUNCTION__, bp);
        place(bp, newsize);
        // user_malloc_size += size; // Add the user-requested size
        return bp;
    }
    #else
    if ((bp = find_fit_best(newsize)) != NULL)
    {
        // mm_check(__FUNCTION__, bp);
        place(bp, newsize);
        // user_malloc_size += size; // Add the user-requested size
        return bp;
    }
    #endif
    /*no fit found.*/
    extend_size = MAX(newsize, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, newsize);
    // user_malloc_size += size; // Add the user-requested size
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    // get utilization
    size_t block_size = GET_SIZE(HDRP(bp));
    user_malloc_size -= block_size - WSIZE; // Subtract user space (block size minus header)

    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    void *head_next_bp = NULL;

    PUT(HDRP(bp), PACK(size, prev_alloc, 0));
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));
    // mm_check(__FUNCTION__, bp);

     /*notify next_block, i am free*/
    head_next_bp = HDRP(NEXT_BLKP(bp));
    PUT(head_next_bp, PACK_PREV_ALLOC(GET(head_next_bp), 0));

    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void *extend_heap(size_t words)
{
    /*get heap_brk*/
    char *old_heap_brk = mem_sbrk(0);
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(old_heap_brk));

    /*printf("\nin extend_heap prev_alloc=%u\n", prev_alloc);*/
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // get utilization
    heap_size += size; // Add the extended heap size

    PUT(HDRP(bp), PACK(size, prev_alloc, 0)); /*last free block*/
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1)); /*break block*/
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    /*
        TODO:
            将 bp 指向的空闲块 与 相邻块合并
            结合前一块及后一块的分配情况，共有 4 种可能性
            分别完成相应case下的 数据结构维护逻辑
    */
    // printf("\nBEFORE coalesce:");
    // mm_check(__FUNCTION__, bp);
    if (prev_alloc && next_alloc) /* 前后都是已分配的块 */
    {
        // 修改头部、脚部； add_to_free_list() 添加到空闲链表(设置前驱和后继)
        PUT(HDRP(bp), PACK(size, prev_alloc, 0));
        PUT(FTRP(bp), PACK(size, prev_alloc, 0));
        add_to_free_list(bp);
    }
    else if (prev_alloc && !next_alloc) /*前块已分配，后块空闲*/
    {
        // 先delete_from_free_list(bp+size2)
        // 修改头部； size = size2 + size3
        // 修改脚部；
        // add_to_free_list() 添加到空闲链表(设置前驱和后继)

        size_t next_alloc_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // char *next_FTRP = FTRP(bp) + next_alloc_size;

        delete_from_free_list(NEXT_BLKP(bp));   // 删除旧的空闲链表
        size = size + next_alloc_size;
        PUT(HDRP(bp), PACK(size, prev_alloc, 0));   // 头部
        PUT(FTRP(bp), PACK(size, prev_alloc, 0));   // 尾部
        add_to_free_list(bp);
    }
    else if (!prev_alloc && next_alloc) /*前块空闲，后块已分配*/
    {
        // 先delete_from_free_list(PREV_BLKP(bp))
        // 更新bp ??
        // 修改头部； size = size1 + size2
        // 修改脚部；
        // add_to_free_list() 添加到空闲链表(设置前驱和后继)
        // 修改邻居块（后块）的prev_alloc域为0 （在母函数中已经做过了）

        void *prev_bp = PREV_BLKP(bp);
        size_t prev_size = GET_SIZE(HDRP(prev_bp));

        delete_from_free_list(prev_bp);   // 删除旧的空闲链表
        bp = prev_bp;              // 更新bp ??
        size = size + prev_size;

        prev_alloc = GET_PREV_ALLOC(HDRP(bp));      // 这步需要更新！
        PUT(HDRP(bp), PACK(size, prev_alloc, 0));   // 头部
        PUT(FTRP(bp), PACK(size, prev_alloc, 0));   // 尾部
        add_to_free_list(bp);
    }
    else /*前后都是空闲块*/
    {
        // 先delete_from_free_list() prev, next
        // 更新bp
        // 修改头部； size = size1 + size2 + size3
        // 修改脚部；
        // add_to_free_list() 添加到空闲链表(设置前驱和后继)

        size_t prev_alloc_size = GET_SIZE(PREV_BLKP(bp) - WSIZE);
        size_t next_alloc_size = GET_SIZE(NEXT_BLKP(bp) - WSIZE);

        // 删除旧的空闲链表
        delete_from_free_list(PREV_BLKP(bp));
        delete_from_free_list(NEXT_BLKP(bp));

        bp = PREV_BLKP(bp);      // 更新bp ??
        size = size + prev_alloc_size + next_alloc_size;

        prev_alloc = GET_PREV_ALLOC(HDRP(bp));      // 这步需要更新！
        PUT(HDRP(bp), PACK(size, prev_alloc, 0));   // 头部
        PUT(FTRP(bp), PACK(size, prev_alloc, 0));   // 尾部
        add_to_free_list(bp);
    }
    // printf("\nAFTER coalesce:");
    // mm_check(__FUNCTION__, bp);
    return bp;
}

#if FIRST_FIT
static void *find_fit_first(size_t asize)
{
    /*
        首次匹配算法
        TODO:
            遍历 freelist， 找到第一个合适的空闲块后返回

        HINT: asize 已经计算了块头部的大小
    */
    void *bp = free_listp;
    if(free_listp == NULL)
        return NULL;
    while (bp != NULL){
        if(GET_SIZE(HDRP(bp)) >= asize)
            return bp;
        bp = (void *)GET_SUCC(bp);      // 更新为bp的后继
    }
    return NULL; // 换成实际返回值
}

#else
static void* find_fit_best(size_t asize) {
    /*
        最佳配算法
        TODO:
            遍历 freelist， 找到最合适的空闲块，返回

        HINT: asize 已经计算了块头部的大小
    */
    // void *bp = free_listp;
    // void *best_bp = NULL;
    // size_t best_size;   // 初值怎么赋值？
    // char best_flag = 0;    // 辅助变量
    // if(free_listp == NULL)
    //     return NULL;
    // while (bp != NULL){
    //     if(GET_SIZE(HDRP(bp)) >= asize){
    //         if(best_flag == 0){
    //             best_flag = 1;
    //             best_size = GET_SIZE(HDRP(bp));
    //             best_bp = bp;
    //         }
    //         if(best_size > GET_SIZE(HDRP(bp))) {
    //             best_size = GET_SIZE(HDRP(bp));
    //             best_bp = bp;
    //         }
    //     }
    //     bp = (void *)GET_SUCC(bp);
    // }
    // return (best_bp); // 换成实际返回值
    
    /*------------version 2----------------*/
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

static void place(void *bp, size_t asize)
{
    /*
        TODO:
        将一个空闲块转变为已分配的块

        HINTS:
            1. 若空闲块在分离出一个 asize 大小的使用块后，剩余空间不足空闲块的最小大小，
                则原先整个空闲块应该都分配出去
            2. 若剩余空间仍可作为一个空闲块，则原空闲块被分割为一个已分配块+一个新的空闲块
            3. 空闲块的最小大小已经 #define，或者根据自己的理解计算该值
    */

    /*------------version of mine----------------*/
    // size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    // size_t size = GET_SIZE(HDRP(bp)); // 预先空间
    // size_t alloc_size;  // 分配空间
    // size_t free_size = size - asize;   // 空闲空间
    // if(free_size <= MIN_BLK_SIZE){
    //     // 剩余空间不足空闲块的最小大小
    //     alloc_size = size;
    // }
    // else {
    //     alloc_size = asize;
    // }

    // PUT(HDRP(bp), PACK(alloc_size, prev_alloc, 1));   // 头部
    // delete_from_free_list(bp);  // 从空闲链表删除

    // if(free_size > MIN_BLK_SIZE){
    //     // 剩余空间足够一个空闲块
    //     void * free_BLK_bp = (void*)NEXT_BLKP(bp);      // 空闲块的bp
    //     PUT(HDRP(free_BLK_bp), PACK(free_size, 1, 0));  // 空闲块头部，前一块是分配块
    //     PUT(FTRP(free_BLK_bp), PACK(free_size, 1, 0));  // 修改空闲块尾部
    //     add_to_free_list(free_BLK_bp);                  // 添加到空闲链表

    // }
    // else{
    //     void *next_bp = NEXT_BLKP(bp);
    //     size_t next_size = GET_SIZE(HDRP(next_bp));
    //     if (next_size != 0) { // 避免尾块溢出
    //         int next_alloc = GET_ALLOC(HDRP(next_bp));
    //         PUT(HDRP(next_bp), PACK(next_size, 1, next_alloc));
    //     }
    // }

    /*------------version 2----------------*/
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t size = GET_SIZE(HDRP(bp));
    size_t free_size = size - asize;
     

    // Remove this block from free list in any case
    delete_from_free_list(bp);

    if (free_size < MIN_BLK_SIZE) {
        // If remaining space is too small for a free block,
        // allocate the entire block
        PUT(HDRP(bp), PACK(size, prev_alloc, 1));

        // Update next block's prev_alloc bit to 1
        void *next_bp = NEXT_BLKP(bp);

        size_t next_size = GET_SIZE(HDRP(next_bp));
        int next_alloc = GET_ALLOC(HDRP(next_bp));
        PUT(HDRP(next_bp), PACK(next_size, 1, next_alloc));

        user_malloc_size += size - WSIZE;
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
        user_malloc_size += asize - WSIZE;
    }
}

static void add_to_free_list(void *bp)
{
    /*set pred & succ*/
    if (free_listp == NULL) /*free_list empty*/
    {
        SET_PRED(bp, 0);
        SET_SUCC(bp, 0);
        free_listp = bp;
    }
    else
    {
        SET_PRED(bp, 0);
        SET_SUCC(bp, (size_t)free_listp); /*size_t ???*/
        SET_PRED(free_listp, (size_t)bp);
        free_listp = bp;
    }
}

static void delete_from_free_list(void *bp)
{
    size_t prev_free_bp=0;
    size_t next_free_bp=0;
    if (free_listp == NULL)
        return;
    prev_free_bp = GET_PRED(bp);
    next_free_bp = GET_SUCC(bp);

    if (prev_free_bp == next_free_bp && prev_free_bp != 0)
    {
        
    }
    if (prev_free_bp && next_free_bp) /*11*/
    {
        SET_SUCC(prev_free_bp, GET_SUCC(bp));
        SET_PRED(next_free_bp, GET_PRED(bp));
    }
    else if (prev_free_bp && !next_free_bp) /*10*/
    {
        SET_SUCC(prev_free_bp, 0);
    }
    else if (!prev_free_bp && next_free_bp) /*01*/
    {
        SET_PRED(next_free_bp, 0);
        free_listp = (void *)next_free_bp;
    }
    else /*00*/
    {
        free_listp = NULL;
    }
}


/*
    mm_check - print the information of the free block beginning with **bp**.
    Please read the experiment tutorial before you use this function.
*/
void mm_check(const char *function, char* bp)
{
    #ifdef DEBUG
    printf("\n---cur func: %s :\n", function);
    printf("addr_start: 0x%lx, addr_end: 0x%lx, size_head: %lu, size_foot: %lu, PRED=0x%lx, SUCC=0x%lx \n", (size_t)bp - WSIZE,
        (size_t)FTRP(bp), GET_SIZE(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_PRED(bp), GET_SUCC(bp));
    #endif
}

/*
 * mm.c - 一个最基础版本的内存分配器，通过 sbrk 预留堆空间，然后只分配，不回收。
 *        每次分配时都直接从当前未使用过的堆空间中分配，没有足够空间时，使用 sbrk 拓展固定大小的堆空间。
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#define MAX_HEAP (2 * 1024 * 1024) // 每次使用 sbrk 拓展的大小，2MB

static char *mem_start_brk;  // 当前堆的起始地址
static char *mem_brk;        // 当前已经使用的堆空间的结束地址（或者说，当前未使用的堆空间的起始地址）
static char *mem_max_addr;   // 当前已分配的堆的结束地址

/* 
 * mm_init - 初始化内存分配器，通过 sbrk 先预留 MAX_HEAP 大小的内存空间
 */
int mm_init(void)
{
    mem_start_brk = (char*)sbrk(MAX_HEAP);
    if (mem_start_brk == NULL) {
        fprintf(stderr, "mem_init: sbrk failed\n");
        exit(1);
    }

    mem_max_addr = mem_start_brk + MAX_HEAP;    // 设置正确的结束地址
    mem_brk = mem_start_brk;                    // 开始未使用任何空间
    return 0;
}

/* 
 * mm_malloc - 分配 size 字节的内存空间，
 */
void *mm_malloc(size_t size)
{
    /*
        TODO:
        从当前未使用的堆空间（mem_brk开始的空间）中分配 size 字节的内存空间。
        如果剩余空间不够，则使用 sbrk 拓展足够的空间（注意对齐）。
    */

    long rest_size = 0; // TODO: 修改该行，计算当前剩余的堆空间大小 （提示，使用全局变量）

    if (rest_size < size) {
        // 如果剩余空间不够，需要使用 sbrk 拓展
        long need_size = size - rest_size;  // 缺少的空间
        long need_size_aligned = (need_size + MAX_HEAP - 1) / MAX_HEAP * MAX_HEAP;  // 上取整到 MAX_HEAP 的倍数

        // TODO: 使用 sbrk 拓展堆 need_size_aligned 字节的空间，并更新 mem_max_addr
        // ...
    }

    // TODO: 修改下面的代码，分配 size 字节的内存空间，并更新 mem_brk，然后返回分配的内存空间的起始地址
    char *bp = NULL;
    // ...
    return (void*)bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    // 不做任何事。
    return;
}

double get_utilization() {
    // 在这个简单的方案中，free时没法得知释放的内存块大小，因此无法计算使用率，我们返回 NaN。
    return (double) (0.0/0.0);
}
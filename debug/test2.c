#define PACK(size, prev_alloc, alloc) (((size) & ~0x7) | ((prev_alloc << 1) & ~0x1) | (alloc)) // In fact, we enforce SIZE to be multiple of 8 :)
#include <stdio.h>

int main()
{
    int a = PACK(16, 1, 1);
    printf("%d\n", a);
    // printf("%b\n", 16&~0x7);
    // 2'b10011
    /*
        头部大小为一个字(64 bits)，
        其中第3-64位存储该块大小的高位。（因为按字对齐，所以低三位都0）
        第0位的值表示该块是否已分配，0表示未分配（空闲块），1表示已分配（分配块）。
        第1位的值表示该块前面的邻居块是否已分配，0表示前邻居未分配，1表示前邻居已分配。

        空闲块包括头部、脚部、前驱、后继
        分配块只用填充头部  size_[2]_[prev]_1
    */
    /*
        1.修改空闲块头部，将空闲块从空闲链表中删除，将大小改为分配的大小，并标记该块为已分配。
        2.为多余的内存添加一个块头部，记录其大小并标记为未分配，正确设置 PREV_ALLOC 域，并将其加入空闲链表，
          使其成为一个新的空闲内存块。(还需要修改尾部？？)
        3.返回分配的块指针。

        在函数的实现过程中，你还需要思考：如果空闲块多余的空间不足以构成一个空闲块，那么还能将其放回空闲
        链表吗？分配后，邻居块的PREV_ALLOC 域应该如何修改？
    */
    void *bp = 0;
    if(bp == 1){
        printf("no\n");
    }
    else
        printf("yes\n");
    // (char *)((char *) bp);
    
    return 0;
}

char* free_listp;
#define WSIZE   8


static void* find_fit_best(size_t asize) {
    /* 
        最佳配算法
        TODO:
            遍历 freelist， 找到最合适的空闲块，返回
        
        HINT: asize 已经计算了块头部的大小
    */
    char *bp = free_listp;
    char *best_bp = NULL;
    size_t best_size;   // 初值怎么赋值？
    void *best_flag = 0;    // 辅助变量
    if(free_listp == NULL)
        return NULL;
    while (1){
        if(GET_SIZE(HDRP(bp)) >= asize){
            if(best_flag == 0){
                best_flag = 1;
                best_size = GET_SIZE(HDRP(bp));
                best_bp = bp;
            }
            if(best_size > GET_SIZE(HDRP(bp))) {
                best_size = GET_SIZE(HDRP(bp));
                best_bp = bp;
            }
        }
        bp = GET_SUCC(bp);
        if(bp == NULL)
            break;
    }
    return ((void *)best_bp); // 换成实际返回值
}
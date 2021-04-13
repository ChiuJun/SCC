//
// Created by LJChi on 2021/3/28.
//

#include "cc.h"

/*空闲内存块*/
static struct memory_block *FreeBlocks;

/*
 * 初始化@param hp指向的 struct heap
 * */
void InitHeap(Heap hp) {
    hp->lastBlockPtr = &hp->headBlock;
    hp->headBlock.next = NULL;
    hp->headBlock.begin = hp->headBlock.avail = hp->headBlock.end = NULL;
}

/*
 * 从当前Heap的最后一个memory_block或者FreeBlocks寻找足够空间并返回
 * 如果空间不够则使用malloc申请新的内存
 * */
void *HeapAllocate(Heap hp, size_t size) {
    size = ALIGN(size, sizeof(union align_util));
    struct memory_block *blk = hp->headBlock.next;
    struct memory_block *preBlk = NULL;

    while (blk != NULL) {
        if (size > (blk->end - blk->avail)) {
            blk = blk->next;
        } else {
            break;
        }
    }
    if (blk == NULL) {/*heap上没有合适的内存块则到FreeBlocks中寻找*/
        preBlk = NULL;
        blk = FreeBlocks;
        while (blk != NULL) {
            if (size > (blk->end - blk->avail)) {
                preBlk = blk;
                blk = blk->next;
            } else {
                if(preBlk == NULL) FreeBlocks = blk->next;
                else preBlk->next = blk->next;
                blk->next = NULL;
                hp->lastBlockPtr->next = blk;
                hp->lastBlockPtr = blk;
                break;
            }
        }
    }
    if (blk == NULL) {/*无空闲内存块则申请新的内存块*/
        size_t blk_size = size + MEMORY_BLOCK_SIZE + sizeof(struct memory_block);
        blk = (struct memory_block *) malloc(blk_size);
        if (blk == NULL) {
            Fatal("Memory exhausted");
        }
        blk->end = (char *) blk + blk_size;
        blk->begin = blk->avail = (char *) (blk + 1);
        blk->next = NULL;
        hp->lastBlockPtr->next = blk;
        hp->lastBlockPtr = blk;
    }

    blk->avail += size;
    return blk->avail - size;
}

/*
 * 释放@param hp指向的 struct heap
 * 并不真正回收空间，只是挂载至FreeBlocks
 * */
void FreeHeap(Heap hp) {
    struct memory_block *blk = hp->headBlock.next;
    while (blk != NULL) {
        blk->avail = blk->begin;
        blk = blk->next;
    }
    hp->lastBlockPtr->next = FreeBlocks;
    FreeBlocks = hp->headBlock.next;
    InitHeap(hp);
}
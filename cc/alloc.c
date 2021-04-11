//
// Created by LJChi on 2021/3/28.
//
#include <stddef.h>
#include <stdlib.h>
#include "cc.h"
#include "alloc.h"
#include "error.h"

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
 * TODO:这种分配策略可能会产生较多内存碎片
 * */
void *HeapAllocate(Heap hp, size_t size) {
    size = ALIGN(size, sizeof(union align_util));
    struct memory_block *blk = hp->lastBlockPtr;

    while (size > (blk->end - blk->avail)) {
        if ((blk->next = FreeBlocks) != NULL) {
            /*首选到空闲内存块中寻找可用内存块*/
            FreeBlocks = FreeBlocks->next;
            blk = blk->next;
        } else {
            /*无空闲内存块则申请新的内存块*/
            size_t blk_size = size + MEMORY_BLOCK_SIZE + sizeof(struct memory_block);
            blk->next = (struct memory_block *) malloc(blk_size);
            blk = blk->next;
            if (blk == NULL) {
                Fatal("Memory exhausted");
            }
            blk->end = (char *) blk + blk_size;
        }
        blk->next = NULL;
        blk->begin = blk->avail = (char *) (blk + 1);
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
    hp->lastBlockPtr->next = FreeBlocks;
    FreeBlocks->next = hp->headBlock.next;
    InitHeap(hp);
}
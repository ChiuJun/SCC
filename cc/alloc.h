//
// Created by LJChi on 2021/3/28.
//

#ifndef SCC_ALLOC_H
#define SCC_ALLOC_H

/*
 * 内存块总可用范围[begin,end)
 * 内存块当前可用范围[avail,end)
 * */
struct memory_block {
    struct memory_block *next;
    /*内存块起始地址*/
    char *begin;
    /*内存块可用地址*/
    char *avail;
    /*内存块结束地址*/
    char *end;
};

typedef struct heap {
    struct memory_block *lastBlockPtr;
    struct memory_block headBlock;
} *Heap;

union align_util {
    double d;
    int (*f)(void);
};

#define MEMORY_BLOCK_SIZE (4 * 1024)
#define HEAP(hp) struct heap hp = { &hp.headBlock }

#define DO_ALLOC(p) ((p) = HeapAllocate(CurrentHeap, sizeof *(p)))
#define ALLOC(p) memset(DO_ALLOC(p), 0, sizeof *(p))
#define CALLOC(p) memset(DO_ALLOC(p), 0, sizeof *(p))

void InitHeap(Heap hp);

void *HeapAllocate(Heap hp, size_t size);

void FreeHeap(Heap hp);

#endif //SCC_ALLOC_H

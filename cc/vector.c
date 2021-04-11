//
// Created by LJChi on 2021/4/11.
//

#include "cc.h"

/*
 * 创建一个 @param capacity 所指定大小的Vector
 * */
Vector CreateVector(size_t capacity) {
    Vector v = NULL;
    ALLOC(v);
    v->data = (void **) HeapAllocate(CurrentHeap, capacity * sizeof(void *));
    v->size = 0;
    v->capacity = capacity;

    return v;
}

/*
 * @param v 指定的Vector扩容,策略为当前capacity的两倍
 * */
void ExpandVector(Vector v) {
    void **oldDataPtr;

    oldDataPtr = v->data;
    v->data = (void **) HeapAllocate(CurrentHeap, v->capacity * 2 * sizeof(void *));
    memcpy(v->data, oldDataPtr, v->capacity * sizeof(void *));
    v->capacity *= 2;
}
//
// Created by LJChi on 2021/4/11.
//

#ifndef SCC_VECTOR_H
#define SCC_VECTOR_H

/*
 * 仿照C++的Vector容器，扩张策略为capacity翻倍
 * data为void *指针数组，分别指向对应的数据
 * size为当前Vector实际使用的大小
 * capacity为当前Vector预分配的大小
 * */
typedef struct vector {
    void **data;
    int size;
    int capacity;
} *Vector;

/*返回v指向的struct vector实际使用的大小*/
#define VECTOR_SIZE(v) (v->size)

#define INSERT_VECTOR_ITEM(v, item) \
do{                                 \
    if(v->size >= v->capacity)      \
        ExpandVector(v);            \
    v->data[v->size] = item;        \
    v->size++;                      \
}while(0)

#define GET_VECTOR_ITEM(v, i) (v->data[i])

#define GET_VECTOR_BACK_ITEM(v) (v->size == 0 ? NULL : v->data[v->size - 1])

#define FOR_EACH_VECTOR_ITEM(itemType,item,v)   \
{                                           \
    int size = v->size;                     \
    int i = 0;                              \
    for(i = 0; i < size; ++i){              \
        item = (itemType)v->data[i];
#define END_FOR_EACH_VECTOR_ITEM    \
    }                               \
}

Vector CreateVector(int size);
void ExpandVector(Vector v);

#endif //SCC_VECTOR_H

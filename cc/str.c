//
// Created by LJChi on 2021/4/15.
//

#include "cc.h"
#include "config.h"

static NameBucket NameBuckets[STRING_HASH_MASK + 1];

static unsigned int ELFHash(char *str, int len) {
    unsigned int hash = 0;
    unsigned int x = 0;

    while (len--){
        hash = (hash << 4) + *str++;
        if ((x = hash & 0xF0000000) != 0) {
            hash ^= x >> 24;
            hash &= ~x;
        }
    }

    return hash & STRING_HASH_MASK;
}

/*
 * 用于存储标志符名称
 * @param len 传入的字符串长度，不包含结尾的'\0'
 * */
char* IdentifierName(char *id, int len) {
    NameBucket ptr;
    int hashValue = ELFHash(id, len);
    for (ptr = NameBuckets[hashValue]; ptr != NULL; ptr = ptr->next) {
        if (len == ptr->len && strncmp(id, ptr->name, len) == 0)
            return ptr->name;
    }

    /*在hash桶里没找到*/
    ptr = HeapAllocate(&StringHeap, sizeof(*ptr));
    ptr->name = HeapAllocate(&StringHeap, len + 1);
    memcpy(ptr->name,id,len);
    ptr->name[len] = 0;
    ptr->len = len;
    ptr->next = NameBuckets[hashValue];
    NameBuckets[hashValue] = ptr;

    return ptr->name;
}

/*
 * 用于存储字符字面量
 * @param len 传入的字符串长度，不包含结尾的'\0'
 * */
void AppendSTR(String str, char *tmp, int len, int wide) {
    int size,charSize = 1;
    char *ptr;

    if (wide) {
        charSize = sizeof(SCC_WCHAR_T);
    }
    size = str->len + len + 1;

    ptr = HeapAllocate(&StringHeap, size * charSize);
    memcpy(ptr,str->content,str->len*charSize);
    memcpy((ptr+str->len*charSize),tmp,len*charSize);
    str->content = ptr;
    str->len = size - 1;
    if (wide) {
        *((SCC_WCHAR_T *)ptr+(size - 1)) = 0;
    } else {
        str->content[size - 1] = 0;
    }
}

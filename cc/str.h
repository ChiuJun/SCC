//
// Created by LJChi on 2021/4/15.
//

#ifndef SCC_STR_H
#define SCC_STR_H

typedef struct nameBucket{
    char *name;
    /*字符串长度，不包含结尾的'\0'*/
    int len;
    struct nameBucket *next;
} *NameBucket;

typedef struct string{
    char *content;
    int len;
} *String;

#define STRING_HASH_MASK 1023

char* IdentifierName(char *id, int len);
void AppendSTR(String str, char *tmp, int len, int wide);

#endif //SCC_STR_H

//
// Created by LJChi on 2021/3/20.
//
#include <stdio.h>
#include "cc.h"

/*用于指向当前操作的Heap:ProgramHeap或者FileHeap*/
Heap CurrentHeap;
/*
 * 用于存放编译器程序相关的信息
 * 存在于整个程序运行期间
 * */
HEAP(ProgramHeap);
/*
 * 用于存放当前处理的文件的相关信息
 * 存在于处理该文件期间
 * */
HEAP(FileHeap);
/*
 * 用于存放程序运行期间的字符串
 * 存在于整个程序运行期间
 * */
HEAP(StringHeap);

static int ParseCommandLine(int argc, char *argv[]){

}

void testVector(void){
    printf("-------------------测试Vector-------------------\n");
    char * str1 = "test str 1";
    char * str2 = "test str 2";
    char * str3 = "test str 3";
    char * tmp;
    Vector v = CreateVector(2);
    printf("创建新的Vector 当前vector size:%d capacity:%d\n",GET_VECTOR_SIZE(v),v->capacity);

    INSERT_VECTOR_ITEM(v,str1);
    printf("插入数据str1[%s]\t",(char *)GET_VECTOR_BACK_ITEM(v));
    printf("当前vector size:%d capacity:%d\n",GET_VECTOR_SIZE(v),v->capacity);

    INSERT_VECTOR_ITEM(v,str2);
    printf("插入数据str2[%s]\t",(char *)GET_VECTOR_ITEM(v,2-1));
    printf("当前vector size:%d capacity:%d\n",GET_VECTOR_SIZE(v),v->capacity);


    INSERT_VECTOR_ITEM(v,str3);
    printf("插入数据str3[%s]\t",(char *)GET_VECTOR_BACK_ITEM(v));
    printf("当前vector size:%d capacity:%d\n",GET_VECTOR_SIZE(v),v->capacity);

    printf("遍历Vector数据\n");
    FOR_EACH_VECTOR_ITEM(char *,tmp,v)
            printf("\t[%s]\n",tmp);
    END_FOR_EACH_VECTOR_ITEM
    printf("遍历Vector数据\n");

    printf("-------------------测试Vector-------------------\n");
}

int main(int argc, char *argv[]) {
    int i;

    CurrentHeap = &ProgramHeap;
    argc--,argv++;

    testVector();

    return 0;
}

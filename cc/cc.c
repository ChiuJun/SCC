//
// Created by LJChi on 2021/3/20.
//
#include <stdio.h>
#include "alloc.h"
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

int main(int argc, char *argv[]) {
    int i;
    argc--,argv++;
    printf("test\n");

    return 0;
}

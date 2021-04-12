//
// Created by LJChi on 2021/3/20.
//
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
/*错误数*/
int ErrorCount;
/*警告数*/
int WarningCount;

static int ParseCommandLine(int argc, char *argv[]){

}

void testInput(void){
    Coord current_coord = NULL;
    ALLOC(current_coord);
    ReadSourceFile("input.c");
    printf("//filename:%s\n//filesize:%ld\n",Input.filename,Input.file_size);
    if(Input.base[Input.file_size] == END_OF_FILE){
        Input.base[Input.file_size] = 0;
    }
    printf("%s\n",Input.base);
    CloseSourceFile();

    current_coord->src_filename = Input.filename;
    current_coord->src_line = current_coord->pp_line = current_coord->col = 1;
    Error(current_coord,"[test error.h]\n");
}

int main(int argc, char *argv[]) {
    int i;

    CurrentHeap = &ProgramHeap;
    argc--,argv++;
    testInput();

    return 0;
}

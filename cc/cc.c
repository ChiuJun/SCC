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
    return 0;
}

void testStr(void){
    char *test = "test 1";
    char *res1 = FormatName("test %d",1);
    char *res2 = FormatName("%s",test);
    assert(res1==res2);

    char *res3 = IdentifierName("test 2",6);
    char *res4 = IdentifierName("test 3",6);
    assert(res3!=res4);

    String str;
    ALLOC(str);
    AppendSTR(str,res1,6,0);
    AppendSTR(str,"append content",strlen("append content"),0);
    assert(str->len==6+strlen("append content"));
}

int main(int argc, char *argv[]) {
    CurrentHeap = &ProgramHeap;
    argc--,argv++;
    ParseCommandLine(argc,argv);
    SetupTypeSystem();

    testStr();

    return 0;
}

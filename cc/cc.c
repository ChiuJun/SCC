//
// Created by LJChi on 2021/3/20.
//
#include "cc.h"

/*输出语法树文件时的文件指针*/
FILE *ASTFile;
/*输出中间代码文件时的文件指针*/
FILE *IRFIle;
/*输出汇编代码文件时的文件指针*/
FILE *ASMFile;
/*汇编代码文件的扩展名*/
char *ASMExtName = ".s";
/*汇编代码文件的文件名*/
char *ASMFileName = NULL;
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

void testOutput(void){
    char * fileName = "output.c";
    ASMFile = CreateOutput("test.c",ASMExtName);
    LeftAlign(ASMFile,4);
    Print("####test module:%s####",fileName);
    PutChar('\n');
    Flush();
    PutString("void testOutput(void){\n"
              "    char * fileName = \"output.c\";\n"
              "    ASMFile = CreateOutput(\"testOut.c\",\".s\");\n"
              "    LeftAlign(ASMFile,4);\n"
              "    Print(\"####test module:%s####\",fileName);\n"
              "    PutChar('\\n');\n"
              "    Flush();\n"
              "    PutString(\"void testOutput(void){\\n\"\n"
              "              \"    char * fileName = \\\"output.c\\\";\\n\"\n"
              "              \"    ASMFile = CreateOutput(\\\"testOut.c\\\",\\\".s\\\");\\n\"\n"
              "              \"    LeftAlign(ASMFile,4);\\n\"\n"
              "              \"    Print(\\\"####test module:%s####\\\",fileName);\\n\"\n"
              "              \"    PutChar('\\\\n');\\n\"\n"
              "              \"    Flush();\\n\"\n"
              "              \"    fclose(ASMFile);\\n\"\n"
              "              \"}\");\n"
              "    Flush();\n"
              "    fclose(ASMFile);\n"
              "}");
    Flush();
    fclose(ASMFile);
}

int main(int argc, char *argv[]) {
    CurrentHeap = &ProgramHeap;
    argc--,argv++;
    ParseCommandLine(argc,argv);
    SetupTypeSystem();

    testOutput();

    return 0;
}

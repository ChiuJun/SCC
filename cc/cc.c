//
// Created by LJChi on 2021/3/20.
//
#include "cc.h"
#include "target.h"

/*是否输出语法树*/
static int DumpAST;
/*是否输出中间代码*/
static int DumpIR;
/*输出语法树文件时的文件指针*/
FILE *ASTFile;
/*输出中间代码文件时的文件指针*/
FILE *IRFile;
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

static int ParseCommandLine(int argc, char *argv[]) {
    /*参见scc.c GenCommand() 输入文件放在参数的最后面*/
    int idx;
    for (idx = 0; idx < argc; ++idx) {
        if (strcmp(argv[idx], "-o") == 0) {
            ++idx;
            ASMFileName = argv[idx];
        } else if (strcmp(argv[idx], "--dump-AST") == 0) {
            DumpAST = 1;
        } else if (strcmp(argv[idx], "--dump-IR") == 0) {
            DumpIR = 1;
        } else {
            return idx;
        }
    }
    return idx;
}

static void Initialize(void) {
    CurrentHeap = &FileHeap;
    ErrorCount = WarningCount = 0;
    ASTFile = IRFile = ASMFile = NULL;
    InitSymbolTable();
}

static void Finalize(void) {
    FreeHeap(&FileHeap);
}

static void Compile(char *file) {
    AstTranslationUnit translationUnit;

    Initialize();

    translationUnit = ParseTranslationUnit(file);
    if(ErrorCount != 0)
        goto exit;

    CheckTranslationUnit(translationUnit);
    if(ErrorCount != 0)
        goto exit;

    Translate(translationUnit);

    if (DumpIR) {
        DisassemblyTranslationUnit(translationUnit);
    }

    EmitTranslationUnit(translationUnit);

    exit:
    Finalize();
}

int main(int argc, char *argv[]) {
    int idx = ParseCommandLine(--argc, ++argv);
    CurrentHeap = &ProgramHeap;

    SetupTypeSystem();
    SetupLexer();
    SetupRegisters();

    while (idx < argc) {
        Compile(argv[idx++]);
    }

    return ErrorCount != 0;
}

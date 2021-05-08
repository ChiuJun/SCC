//
// Created by LJChi on 2021/3/20.
//
#include "cc.h"

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

int CurrentToken;

static int ParseCommandLine(int argc, char *argv[]) {
    /*参见scc.c GenCommand() 输入文件放在参数的最后面*/
    int idx;
    for (idx = 0; idx < argc; ++idx) {
        if (strcmp(argv[idx], "-o") == 0) {
            ++idx;
            ASMFileName = argv[idx];
        } else if (strcmp(argv[idx], "--dump-ast") == 0) {
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
    Initialize();
    ReadSourceFile(file);
    TokenCoord.src_filename = Input.filename;
    TokenCoord.pp_line = TokenCoord.src_line = TokenCoord.col = 1;
    printf("------------%s------------\n", TokenCoord.src_filename);
    while ((CurrentToken = GetNextToken()) != TK_END) {
        printf("token:%d %s", CurrentToken-1, TokenStrings[CurrentToken-1]);
        switch (CurrentToken) {
            case TK_ID:
                printf("----------------------token value:%s",TokenValue.p);
                break;
            case TK_INTCONST:
            case TK_UINTCONST:
            case TK_LONGCONST:
            case TK_ULONGCONST:
            case TK_LLONGCONST:
            case TK_ULLONGCONST:
                printf("----------------------token value:%d",TokenValue.i[0]);
                break;
            case TK_FLOATCONST:
                printf("----------------------token value:%f",TokenValue.f);
                break;
            case TK_DOUBLECONST:
            case TK_LDOUBLECONST:
                printf("----------------------token value:%lf",TokenValue.d);
                break;
            case TK_STRING:
                printf("----------------------token value:%s",((String)TokenValue.p)->content);
                break;
            case TK_WIDESTRING:
                printf("----------------------token value:%ls",((String)TokenValue.p)->content);
                break;
            default:
                break;
        }
        printf("\n\t(pp_line:%d src_line:%d col:%d)\n",TokenCoord.pp_line,TokenCoord.src_line,TokenCoord.col);
    }
    CloseSourceFile();
    Finalize();
}

int main(int argc, char *argv[]) {
    int idx = ParseCommandLine(--argc, ++argv);
    CurrentHeap = &ProgramHeap;

    SetupTypeSystem();
    SetupLexer();

    while (idx < argc) {
        Compile(argv[idx++]);
    }

    return ErrorCount != 0;
}

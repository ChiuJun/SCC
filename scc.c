//
// Created by LJChi on 2021/3/8.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scc.h"

struct option Option;
static char **cmd;

static void ShowHelp(void) {
    char *msg = {
            "Usage: scc [options] file...\n"
            "Options:\n"
            "  -E             Preprocess only; do not compile, assemble or link.\n"
            "  -S             Compile only; do not assemble or link.\n"
            "  -c             Compile and assemble, but do not link.\n"
            "  -o <file>      Place the output into <file>.\n"
            "  -Idir          Add dir to the include file search directories.\n"
            "  -Dname=def     Define preprocess macro 'name',its value is 'def'.\n"
            "  -Uname         Undefine the preprocess macro 'name'.\n"
            "  -h             Show this help information.\n"
            "  -v             Display the programs invoked by the compiler.\n"
            "  --dump-ast     Dump syntax tree which is put into a file named xxx.ast.\n"
            "  --dump-IR      Dump intermediate code which is put into a file named xxx.uil.\n"
            "  -Wa,<options>  Pass comma-separated <options> on to the assembler.\n"
            "  -Wl,<options>  Pass comma-separated <options> on to the linker.\n"
            "All other options after first file will be passed to the linker.\n"
    };
    printf("%s", msg);
}

static void *Alloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Memory exhausted!\n");
        exit(-1);
    }

    return ptr;
}

//在 @list 尾部添加一个List节点，值为 @str;
static List ListAppend(List list, char *str) {
    List *tail = &list;
    List ptr = list;
    while (ptr != NULL) {
        tail = &(ptr->next);
        ptr = ptr->next;
    }
    *tail = (List) Alloc(sizeof(*ptr));
    (*tail)->str = str;
    (*tail)->next = NULL;

    return list;
}

static List ListCombine(List from, List to){
    List *tail=&to;
    List ptr=to;
    while(ptr!=NULL){
        tail=&(ptr->next);
        ptr=ptr->next;
    }
    *tail=from;

    return to;
}

static int GetListLen(List list){
    int cnt=0;
    while(list!=NULL){
        cnt++;
        list=list->next;
    }

    return cnt;
}

static int GetFileType(char *filename){

}

static void AddFile(char *filename){

}

static void RemoveFile(void){

}

static void ParseCmdLine(int argc,char *argv[]){
    int i=0;
    for (i = 0; i <argc; ++i) {
        if(!strcmp(argv[i],"-E")){
            Option.output_file_type=PP_FILE;
        }else if(!strcmp(argv[i],"-S")){
            Option.output_file_type=ASM_FILE;
        }else if(!strcmp(argv[i],"-c")){
            Option.output_file_type=OBJ_FILE;
        }else if(!strcmp(argv[i],"-o")){
            Option.out_file_name=argv[++i];
        }else if(!strncmp(argv[i],"-I",2)||
                !strncmp(argv[i],"-D",2)||
                !strncmp(argv[i],"-U",2)){
            Option.p_flags=ListAppend(Option.p_flags,argv[i]);
        }else if(!strcmp(argv[i],"-h")){
            ShowHelp();
            exit(0);
        }else if(!strcmp(argv[i],"-v")){
            Option.verbose=1;
        }else if(!strcmp(argv[i],"--dump-ast")||
                !strcmp(argv[i],"--dump-IR")){
            Option.c_flags=ListAppend(Option.c_flags,argv[i]);
        }else if(!strncmp(argv[i],"-Wa,",4)){
            Option.a_flags=ListAppend(Option.a_flags,argv[i]);
        }else if(!strncmp(argv[i],"-Wl,",4)){
            Option.l_flags=ListAppend(Option.l_flags,argv[i]);
        }else if(argv[i][0]=='-'){//所有其他选项都传递给连接器
            Option.l_input=ListAppend(Option.l_input,argv[i]);
        }else{
            AddFile(argv[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        ShowHelp();
        exit(0);
    }
    
    //TODO:为什么加60
    cmd = (char **)Alloc((argc+60)*(sizeof(char *)));
    cmd[0] = NULL;
    Option.output_file_type=EXE_FILE;
    ParseCmdLine(--argc,++argv);

    return 0;
}
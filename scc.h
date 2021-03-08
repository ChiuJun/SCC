//
// Created by LJChi on 2021/3/8.
//

#ifndef SCC_SCC_H
#define SCC_SCC_H

// PP_FILE:PreprocessFile
// TODO:gcc貌似只能生成动态链接库，所以ExtNames只写了.so
enum {
    C_FILE, PP_FILE, ASM_FILE, OBJ_FILE, LIB_FILE, EXE_FILE
};

char *ExtNames[] = {".c", ".i", ".s", ".o", ".a;.so", NULL};

typedef struct list {
    char *str;
    struct list *next;
} *List;

struct option {
    List p_flags;//preprocessor
    List c_flags;//compiler
    List a_flags;//assembler
    List l_flags;//linker
    List c_files;//.c
    List p_files;//.i
    List a_files;//.s
    List o_files;//.o
    List l_files;//.a .lib
    List l_input;//input for linker
    int verbose;//verbose mode flag
    int output_file_type;//output file type
    char *out_file_name;
};

#endif //SCC_SCC_H
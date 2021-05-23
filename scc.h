/**
 * Created by LJChi on 2021/3/8.
 */

#ifndef SCC_SCC_H
#define SCC_SCC_H

#define SCC_DIR "/home/jiabi/SCC/bin/"

/**
 * PP_FILE:PreprocessFile
 */
enum {
    C_FILE, PP_FILE, ASM_FILE, OBJ_FILE, LIB_FILE, EXE_FILE
};

char *ExtNames[] = {".c", ".i", ".s", ".o", ".so", NULL};

typedef struct list {
    char *str;
    struct list *next;
} *List;

struct option {
    List p_flags;/**preprocessor*/
    List c_flags;/**compiler*/
    List a_flags;/**assembler*/
    List l_flags;/**linker*/
    List c_files;/**.c*/
    List p_files;/**.i*/
    List a_files;/**.s*/
    List o_files;/**.o*/
    List l_files;/**.a .so*/
    int verbose;/**verbose mode flag*/
    int output_file_type;
    char *out_file_name;
};

char *CPPCmd[] =
        {
                "/usr/bin/gcc", "-m32", "-U__GNUC__", "-D_POSIX_SOURCE", "-D__STRICT_ANSI__",
                "-Dunix", "-Di386", "-Dlinux", "-D__unix__", "-D__i386__", "-D__linux__",
                "-D__signed__=signed", "-I" SCC_DIR "include", "$1", "-E", "$2", "-o", "$3", 0
        };

char *CCCmd[] =
        {
                SCC_DIR "cc", "-o", "$3", "$1", "$2", 0
        };

char *ASCmd[] =
        {
                "/usr/bin/gcc", "-m32", "-c", "-o", "$3", "$1", "$2", 0
        };

char *LDCmd[] =
        {
                "/usr/bin/gcc", "-m32", "-o", "$3", "$1", "$2", SCC_DIR "assert.o", "-lc", "-lm", 0
        };

#endif
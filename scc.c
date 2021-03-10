/*
 * Created by LJChi on 2021/3/8.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "scc.h"

static struct option Option;
static char **commands;
static List PPFiles;
static List ASMFiles;
static List OBJFIles;

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

/*
 * 在 @list 尾部添加一个List节点，值为 @str;
 */
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

static List ListCombine(List from, List to) {
    List *tail = &to;
    List ptr = to;
    if (from == NULL)
        return to;
    while (ptr != NULL) {
        tail = &(ptr->next);
        ptr = ptr->next;
    }
    *tail = from;

    return to;
}

static int GetListLen(List list) {
    int cnt = 0;
    while (list != NULL) {
        cnt++;
        list = list->next;
    }

    return cnt;
}

static char *GenFileName(char *filename, char *extname) {
    char *result;
    int len, dot_p;

    if (extname == NULL)
        return filename;

    len = strlen(filename);
    dot_p = len - 1;
    while (filename[dot_p] != '.' && dot_p != -1) {
        dot_p--;
    }
    if (dot_p == -1) dot_p = len;
    len = strlen(extname);
    result = Alloc((dot_p + len + 1) * sizeof(char));
    strncpy(result, filename, dot_p);
    strcat(result, extname);

    return result;
}

/*
 * TODO: 暂时不支持.so文件
 */
static int GetFileType(char *filename) {
    int idx, len_ext;
    int len_file = strlen(filename);
    for (idx = C_FILE; idx <= LIB_FILE; ++idx) {
        len_ext = strlen(ExtNames[idx]);
        if (!strncmp(ExtNames[idx], &filename[len_file - len_ext], len_ext))
            return idx;
    }

    return -1;
}

static void AddFile(char *filename) {
    int type = GetFileType(filename);
    if (type == -1) {
        fprintf(stderr, "%s: Unrecognized file\n", filename);
        return;
    }

    if (access(filename, R_OK)) {
        fprintf(stderr, "%s: No such file\n", filename);
        return;
    }
    switch (type) {
        case C_FILE:
            Option.c_files = ListAppend(Option.c_files, filename);
            break;
        case PP_FILE:
            Option.p_files = ListAppend(Option.p_files, filename);
            break;
        case ASM_FILE:
            Option.a_files = ListAppend(Option.a_files, filename);
            break;
        case OBJ_FILE:
            Option.o_files = ListAppend(Option.o_files, filename);
            break;
        case LIB_FILE:
            Option.l_files = ListAppend(Option.l_files, filename);
            break;
    }
}

static void CleanFiles(void) {
    List ptr;
    if (Option.output_file_type > PP_FILE)
        for (ptr = PPFiles; ptr != NULL; ptr = ptr->next)
            remove(ptr->str);
    if (Option.output_file_type > ASM_FILE)
        for (ptr = ASMFiles; ptr != NULL; ptr = ptr->next)
            remove(ptr->str);
    if (Option.output_file_type > OBJ_FILE)
        for (ptr = OBJFIles; ptr != NULL; ptr = ptr->next)
            remove(ptr->str);
}

static void ParseCmdLine(int argc, char *argv[]) {
    int i = 0;
    for (i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "-E")) {
            Option.output_file_type = PP_FILE;
        } else if (!strcmp(argv[i], "-S")) {
            Option.output_file_type = ASM_FILE;
        } else if (!strcmp(argv[i], "-c")) {
            Option.output_file_type = OBJ_FILE;
        } else if (!strcmp(argv[i], "-o")) {
            Option.out_file_name = argv[++i];
        } else if (!strncmp(argv[i], "-I", 2) ||
                   !strncmp(argv[i], "-D", 2) ||
                   !strncmp(argv[i], "-U", 2)) {
            Option.p_flags = ListAppend(Option.p_flags, argv[i]);
        } else if (!strcmp(argv[i], "-h")) {
            ShowHelp();
            exit(0);
        } else if (!strcmp(argv[i], "-v")) {
            Option.verbose = 1;
        } else if (!strcmp(argv[i], "--dump-ast") ||
                   !strcmp(argv[i], "--dump-IR")) {
            Option.c_flags = ListAppend(Option.c_flags, argv[i]);
        } else if (!strncmp(argv[i], "-Wa,", 4)) {
            Option.a_flags = ListAppend(Option.a_flags, argv[i]);
        } else if (!strncmp(argv[i], "-Wl,", 4)) {
            Option.l_flags = ListAppend(Option.l_flags, argv[i]);
        } else if (argv[i][0] == '-') {/*所有其他选项都传递给连接器*/
            Option.l_flags = ListAppend(Option.l_flags, argv[i]);
        } else {
            AddFile(argv[i]);
        }
    }
}

static int ExecuteCommand(char **cmd) {
    int pid, ret, status;

    pid = fork();
    if (pid == -1) {
        fprintf(stderr, "fork error,no more processes\n");
        return 100;
    } else if (pid == 0) {/*子进程*/
        execv(cmd[0], cmd);
        perror(cmd[0]);
        fflush(stdout);
        exit(100);
    }
    /*父进程*/
    ret = waitpid(pid, &status, 0);
    status = ret == -1 ? ret : status;
    if (status & 0xff) {
        fprintf(stderr, "fatal error in %s\n", cmd[0]);
        status |= 0x100;
    }
    return (status >> 8) & 0xff;
}

static void PrintCommand(void) {
    if (commands[0] == NULL) return;

    int i;
    printf("%s", commands[0]);
    for (i = 1; commands[i] != NULL; ++i) {
        printf(" %s", commands[i]);
    }
    printf("\n");
}

static char **GenCommand(char **cmd, List flags, List in_files, List out_files) {
    int i, j;
    List listArr[3], tmpList;
    listArr[0] = flags;/*$1*/
    listArr[1] = in_files;/*$2*/
    listArr[2] = out_files;/*$3*/

    for (i = j = 0; cmd[i] != NULL; ++i) {
        if (strchr(cmd[i], '$') != NULL) {
            tmpList = listArr[(cmd[i][1] - '0') - 1];
            while (tmpList != NULL) {
                commands[j++] = tmpList->str;
                tmpList = tmpList->next;
            }
        } else {
            commands[j++] = cmd[i];
        }
    }
    commands[j] = NULL;

    if (Option.verbose == 1)
        PrintCommand();

    return commands;
}

static List ParseOption(char *opt) {
    List split_option = NULL;
    int len = strlen(opt);
    char *str = (char *) Alloc(len * sizeof(char));
    char *tmp;
    strcpy(str, opt);
    tmp = strtok(str, ",");
    while (tmp != NULL) {
        split_option = ListAppend(split_option, tmp);
        tmp = strtok(NULL, ",");
    }

    return split_option;
}

static int InvokeProgram(int filetype) {
    int status = 0;
    List ptr, in_files, out_files;
    char *filename, **cmd;

    switch (filetype) {
        case PP_FILE:
            for (ptr = Option.c_files; ptr != NULL; ptr = ptr->next) {
                if (Option.output_file_type == PP_FILE && Option.out_file_name) {
                    /*Option.output_file_type==PP_FILE时
                     * GetListLen(Option.c_files)==1
                     * */
                    filename = Option.out_file_name;
                } else {
                    filename = GenFileName(ptr->str, ExtNames[PP_FILE]);
                }
                in_files = ListAppend(NULL, ptr->str);
                out_files = ListAppend(NULL, filename);
                cmd = GenCommand(CPPCmd, Option.p_flags, in_files, out_files);
                status = ExecuteCommand(cmd);
                PPFiles = ListAppend(PPFiles, filename);
            }
            Option.p_files = ListCombine(PPFiles, Option.p_files);
            break;
        case ASM_FILE:
            for (ptr = Option.p_files; ptr != NULL; ptr = ptr->next) {
                if (Option.output_file_type == ASM_FILE && Option.out_file_name) {
                    filename = Option.out_file_name;
                } else {
                    filename = GenFileName(ptr->str, ExtNames[ASM_FILE]);
                }
                in_files = ListAppend(NULL, ptr->str);
                out_files = ListAppend(NULL, filename);
                cmd = GenCommand(CCCmd, Option.c_flags, in_files, out_files);
                status = ExecuteCommand(cmd);
                ASMFiles = ListAppend(ASMFiles, filename);
            }
            Option.a_files = ListCombine(ASMFiles, Option.a_files);
            break;
        case OBJ_FILE:
            for (ptr = Option.a_flags, Option.a_flags = NULL; ptr != NULL; ptr = ptr->next) {
                Option.a_flags = ListCombine(ParseOption((ptr->str) + 4), Option.a_flags);
            }
            for (ptr = Option.a_files; ptr != NULL; ptr = ptr->next) {
                if (Option.output_file_type == OBJ_FILE && Option.out_file_name) {
                    filename = Option.out_file_name;
                } else {
                    filename = GenFileName(ptr->str, ExtNames[OBJ_FILE]);
                }
                in_files = ListAppend(NULL, ptr->str);
                out_files = ListAppend(NULL, filename);
                cmd = GenCommand(ASCmd, Option.a_flags, in_files, out_files);
                status = ExecuteCommand(cmd);
                OBJFIles = ListAppend(OBJFIles, filename);
            }
            Option.o_files = ListCombine(OBJFIles, Option.o_files);
            break;
        case LIB_FILE:
            /*TODO:暂时不支持生成.so*/
            return 0;
            break;
        case EXE_FILE:
            if (Option.out_file_name == NULL) {
                Option.out_file_name = GenFileName("a.out", ExtNames[EXE_FILE]);
            }
            for (ptr = Option.l_flags, Option.l_flags = NULL; ptr != NULL; ptr = ptr->next) {
                Option.l_flags = ListCombine(ParseOption(ptr->str), Option.l_flags);
            }
            cmd = GenCommand(LDCmd, Option.l_flags, Option.o_files, ListAppend(NULL, Option.out_file_name));
            status = ExecuteCommand(cmd);
            break;
    }

    return status;
}

static void TestScc(void) {
    assert(0 == GetFileType("test.c"));
    assert(1 == GetFileType("test.i"));
    assert(2 == GetFileType("test.s"));
    assert(3 == GetFileType("test.o"));
    assert(4 == GetFileType("test.so"));
    assert(-1 == GetFileType("test.a"));
    assert(!strcmp(GenFileName("test.c", ".o"), "test.o"));
    assert(!strcmp(GenFileName("test.test.c", ".o"), "test.test.o"));
    assert(!strcmp(GenFileName("test", ".o"), "test.o"));
    assert(!strcmp(GenFileName("test.cpp", ".o"), "test.o"));
    assert(!strcmp(GenFileName("test.c", NULL), "test.c"));
    assert(!strcmp(GenFileName("test.c", 0), "test.c"));
    assert(!strcmp(GenFileName("test.c", ".so"), "test.so"));
    assert(!strcmp(GenFileName("test.c", "."), "test."));
    exit(0);
}

int main(int argc, char *argv[]) {
    int filetype;
    if (argc < 2) {
        ShowHelp();
        return 0;
    }

    commands = (char **) Alloc((argc + 100) * (sizeof(char *)));
    commands[0] = NULL;
    Option.output_file_type = EXE_FILE;
    ParseCmdLine(--argc, ++argv);
    if (Option.out_file_name && Option.output_file_type != EXE_FILE &&
        (GetListLen(Option.c_files) > 1 ||
         GetListLen(Option.p_files) > 1 ||
         GetListLen(Option.a_files) > 1)) {
        fprintf(stderr, "cannot specify -o with -c, -S or -E with multiple files\n");
        return -1;
    }

    for (filetype = PP_FILE; filetype <= Option.output_file_type; ++filetype) {
        if (InvokeProgram(filetype)) {
            CleanFiles();
            fprintf(stderr, "scc invoke command error:\n");
            PrintCommand();
            return -1;
        }
    }
    CleanFiles();

    return 0;
}
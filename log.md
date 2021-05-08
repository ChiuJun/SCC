# 毕设日志
> 简要记录毕设开发过程
## 0301日志
### 环境准备  
- 操作系统：Windows 10 Linux Subsystem Ubuntu 18.04.5 LTS
- 编译工具：gcc (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0
> 由于目标为实现C90标准，并生成x86程序，需要安装```gcc-multilib```以及一系列32位库
## 0302日志
### 编写驱动程序
- 毕设实现C编译器，而其他部分调用GCC工具链中的C预处理器、汇编器、链接器。因此需要一个驱动程序作为界面与用户交互并调度工具链中相应的工具。
- 前置知识
    > [unistd.h](https://zh.wikipedia.org/wiki/Unistd.h) 是 C 和 C++ 程序设计语言中提供对 POSIX 操作系统 API 的访问功能的头文件的名称。是Unix Standard的缩写。
    - 参考《UNIX环境高级编程》
    - 在进程中调用其他程序：
        1. 使用```pid_t fork(void);```创建子进程
        2. 在子进程中使用```exec```函数族。
## 0303日志
- 阅读《UNIX环境高级编程》
    - 功能测试宏
        - 注：所有功能测试宏(Feature-Test-Macro)以下划线开始
        > 如果在编译一个程序时，希望它只与POSIX的定义相关，而不与任何实现定义的常量冲突，那么就需要定义常量_POSIX_C_SOURCE。一旦定义了_POSIX_C_SOURCE，所有POSIX.1头文件都使用此常量来排除任何实现专有的定义。  
            POSIX.1 标准的早期版本定义了_POSIX_SOURCE常量。在POSIX.1的2001版中，它被替换为_POSIX_C_SOURCE。
        - 参考[功能测试宏(Feature-Test-Macro)](http://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html)
        > Macro: _POSIX_SOURCE  
        If you define this macro, then the functionality from the POSIX.1 standard (IEEE Standard 1003.1) is available, as well as all of the ISO C facilities.  
        The state of _POSIX_SOURCE is irrelevant if you define the macro _POSIX_C_SOURCE to a positive integer.
    - 关于一些库函数
        - setjmp与longjmp可以用于错误处理  
        ```int setjmp(jmp_buf env);```  
        ```void longjmp(jmp_buf env, int val);```
        - 通过```void *malloc(size_t size);```申请的存储区中的值未初始化，而通过```void *calloc(size_t nmemb, size_t size);```申请的存储区被初始化未全0
    - 进程控制
        - ```pid_t fork(void);```一次调用两次返回
            - 对于父进程，他返回子进程pid
            - 对于子进程，他返回0
            - 父进程和子进程继续执行fork调用之后的指令
            - fork之后父进程还是子进程先执行取决于内核的调度算法
            - 子进程是父进程的副本，拥有独立的存储空间。父进程、子进程共享(只读)正文段以及父进程打开的文件描述符(共享文件)。
            - 每个进程可以使用```pid_t getpid(void);```获取自己的pid，使用```pid_t getppid(void);```获取父进程的pid。
        - ```pid_t vfork(void);```(SUSv3标记为弃用、SUSv4被删除)与```pid_t fork(void);```的区别
            - 并不将父进程的地址空间完全复制到子进程中，在子进程调用```exec```或```exit```之前，它在父进程的空间中运行。
            - ```pid_t vfork(void);```保证子进程先运行，直到子进程调用```exec```或```exit```之后(注意要避免产生死锁)。
        - ```wait```函数族
            ```c
            pid_t wait(int *wstatus);
            pid_t waitpid(pid_t pid, int *wstatus, int options);
            int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
            ```
            - 在一个子进程终止前，```wait```使其调用者阻塞，```waitpid```有一个可选项立即返回0，使其调用者不阻塞。
            - ```waitpid```可以控制所等待的进程
            - ```waitid```与```waitpid```类似，但提供了更多的灵活性。
            - 可以使用fork两次的技巧使得父进程不用阻塞等待其子进程返回，也不会产生僵死(zombie)进程。
        - ```exec```函数族
            ```c
            int execl(const char *path, const char *arg, ...
                            /* (char  *) NULL */);
            int execlp(const char *file, const char *arg, ...
                            /* (char  *) NULL */);
            int execle(const char *path, const char *arg, ...
                            /*, (char *) NULL, char * const envp[] */);
            int execv(const char *path, char *const argv[]);
            int execvp(const char *file, char *const argv[]);
            int execvpe(const char *file, char *const argv[], char *const envp[]);
            int fexecve(int fd, char *const argv[], char *const envp[]);                
            ```
            - 函数名带p的表示该函数取```const char *file```作为参数，并且到```PATH```环境变量指定的目录中寻找可执行文件，如果寻找到的不是可执行文件则作为shell脚本交给```/bin/sh```执行。
            - ```l```表示参数表，```v```表示矢量,两者互斥。
            - ```e```表示取```char *const envp[]```而不使用当前环境。
- 定义Word多级列表，设计论文骨架
## 0308日志
- 测试ucc对于动态链接库的支持  
    ucc将```'-'```开头的未识别选项传递给连接器LD
    - 支持通过gcc的-shared -fPIC选项生成动态链接库
    - 支持通过gcc的-L -l选项使用动态链接库
    - 不识别.so文件
    - 将ucc生成的.o文件与ucc生成的.so文件链接时会报错
    ```bash
    # .
    # ├── Makefile
    # ├── hello.c
    # ├── test.c
    # ├── test.h
    # └── test.sh
    gcc -shared -fPIC -o libg.so test.c -m32 
    ucc -shared -fPIC -o libu.so test.c
    gcc -c hello.c -o g.o -m32 
    ucc -c hello.c -o u.o
    sudo cp libg.so /usr/lib32
    sudo cp libu.so /usr/lib32

    gcc -o guu u.o -L/usr/lib32 -lu -m32
    gcc -o gug u.o -L/usr/lib32 -lg -m32
    gcc -o ggu g.o -L/usr/lib32 -lu -m32
    gcc -o ggg g.o -L/usr/lib32 -lg -m32
    ucc -o uuu u.o -L/usr/lib32 -lu
    ucc -o uug u.o -L/usr/lib32 -lg
    ucc -o ugu g.o -L/usr/lib32 -lu
    ucc -o ugg g.o -L/usr/lib32 -lg

    gcc -o gg hello.c ./libg.so -m32
    gcc -o gu hello.c ./libu.so -m32
    ucc -o ug hello.c ./libg.so
    ucc -o uu hello.c ./libu.so
    ```
- 静态库没有进行测试
## 0309日志
- 构建工具CMake
    - 使用```add_compile_options()```指定编译选项时会连接器会报错
        > /usr/bin/ld: i386 architecture of input file `CMakeFiles/hello.dir/hello.c.o' is incompatible with i386:x86-64 output 
    - 使用```make VERBOSE=1```发现CMake没有给ld连接器传入适当的参数
    - 改用```set(CMAKE_C_FLAGS "-m32 ${CMAKE_C_FLAGS}")```问题解决
- 库函数```int access(const char *pathname, int mode);```，指定参数```mode```为```R_OK```测试指定文件是否存在且具备读权限。
## 0313日志
### x86汇编语言
> 一个x86-64的中央处理单元(CPU)包含一组16个存储64位值的通用目的寄存器，这些寄存器用来存储整数数据和指针。最初的8086中有8个16位的寄存器，每个寄存器都有特殊的用途。扩展到IA32时，这些寄存器也扩展为32位寄存器，标号从%eax到%esp，扩展到x86-64后，原来的8个寄存器扩展成64位，标号从%rax到%rsp。除此之外，还增加了8个新的寄存器，它们的标号是按照新的命名规则定制的，从%r8到%r15。  

### _cdecl  
- cdecl(C declaration，即C声明)是源起C语言的一种调用约定，也是C语言的事实上的标准。在x86架构上，其内容包括：
1. 函数实参在线程栈上按照从右至左的顺序依次压栈。
2. 函数结果保存在寄存器EAX/AX/AL中
3. 浮点型结果存放在寄存器ST0中
4. 编译后的函数名前缀以一个下划线字符
5. 调用者负责从线程栈中弹出实参（即清栈）
6. 8比特或者16比特长的整形实参提升为32比特长。
7. 受到函数调用影响的寄存器（volatile registers）：EAX, ECX, EDX, ST0 - ST7, ES, GS
8. 不受函数调用影响的寄存器： EBX, EBP, ESP, EDI, ESI, CS, DS
9. RET指令从函数被调用者返回到调用者（实质上是读取寄存器EBP所指的线程栈之处保存的函数返回地址并加载到IP寄存器）  

| 寄存器名 | 约定 | 备注 |
| :-----: | :--: | :-- |
| %eax | 易失寄存器（caller saved） | Accumulator |
| %ebx | 保值寄存器（callee saved） | Base |
| %ecx | 易失寄存器（caller saved） | Counter |
| %edx | 易失寄存器（caller saved） | Data |
| %esi | 保值寄存器（callee saved） | Source Index  |
| %edi | 保值寄存器（callee saved） | Destination Index  |
| %ebp | 保值寄存器（callee saved） | Base Pointer |
| %esp | 保值寄存器（callee saved） | Stack Pointer |
### 浮点数操作
- 可以参考Intel® 64 and IA-32 Architectures Software Developer’s Manual的[CHAPTER 8 PROGRAMMING WITH THE X87 FPU](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- 好繁琐

## 0320日志
### C 文件操作
- ```FILE *fopen(const char *pathname, const char *mode);```
> The  fopen()  function  opens the file whose name is the string pointed to by pathname and associates a stream with it.
- ```int fseek(FILE *stream, long offset, int whence);```  
    ```long ftell(FILE *stream);```
> The  fseek() function sets the file position indicator for the stream pointed to by stream.  The new position,measured in bytes, is obtained by adding offset bytes to the position specified by whence.  If whence  is  set to  SEEK_SET,  SEEK_CUR,  or  SEEK_END,  the offset is relative to the start of the file, the current position indicator, or end-of-file, respectively.  A successful call to the fseek()  function  clears  the  end-of-file indicator for the stream and undoes any effects of the ungetc(3) function on the same stream.  
The  ftell()  function  obtains  the current value of the file position indicator for the stream pointed to by stream.
- ```size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);```
> The  function  fread()  reads nmemb items of data, each size bytes long, from the stream pointed to by stream,storing them at the location given by ptr.
### C语言中的类型
1. object types
2. function types
3. incomplete types

- 基本类型(basic types)
    - char, short int, int, long int
    - float, double, long double
- 派生类型(derived types)
    - array type
    - structure type
    - union type
    - function type
    - pointer type

- 类型提升
    - 在ANSI C中，如果使用了适当的函数原型，类型提升便不会发生，否则(无函数原型或者对应的参数不匹配)也会发生。

## 0328日志
### 内存管理模块
- 对齐要求
> 3.1 alignment A requirement that objects of a particular type be located on storage boudaries with addresses that are particular multiplies of a byte address.
- 聚合初始化
> The initializer for a structure or union object that has automatic storage duration either shall be a initializer list as described below, or shall be a single expression that has compatible struct or union type. In latter case, the initial calue of the object is that of the expression.
- 程序分为3个Heap：ProgramHeap、FileHeap、StringHeap
```c
/*
 * 内存块总可用范围[begin,end)
 * 内存块当前可用范围[avail,end)
 * */
struct memory_block {
    struct memory_block *next;
    /*内存块起始地址*/
    char *begin;
    /*内存块可用地址*/
    char *avail;
    /*内存块结束地址*/
    char *end;
};

typedef struct heap {
    struct memory_block *lastBlockPtr;
    struct memory_block headBlock;
} *Heap;
```
## 0411日志
- 公共工具
    - input.c 文件输入
    - output.c 文件输出
    - vector.c Vector类型支持  
    - error.c 错误报告
- 基础模块
    - alloc.c 内存管理
    - type.c 类型子系统
    - symbol.c 符号表管理

### Vector
```c
/*
 * 仿照C++的Vector容器，扩张策略为capacity翻倍
 * data为void *指针数组，分别指向对应的数据
 * size为当前Vector实际使用的大小
 * capacity为当前Vector预分配的大小
 * */
typedef struct vector {
    void **data;
    int size;
    int capacity;
} *Vector;
```

## 0412日志
### 文件输入模块&错误报告模块
```c
/*
 * 坐标 记录当前处理的位置
 * 用于出错时指示出错位置
 * */
typedef struct coord{
    /*源文件文件名*/
    char *src_filename;
    /*源文件行号*/
    int src_line;
    /*预处理文件的行号*/
    int pp_line;
    /*预处理文件的列*/
    int col;
} *Coord;

struct input{
    /*文件名*/
    char *filename;
    /*对应文件在内存中的起始位置*/
    unsigned char *base;
    /*当前处理的位置*/
    unsigned char *cursor;
    /*当前行的头部位置*/
    unsigned char *line_head;
    /*当前处理的行*/
    int line;
    /*文件大小*/
    unsigned long file_size;
};
```
- 完成论文2.1节

## 0413日志
### 类型子系统
```c
#define TYPE_COMMON     \
    int category : 8;   \
    int qualify : 8;    \
    int align   : 16;   \
    int size;           \
    struct type *base_type;

typedef struct field {
    char *field_id;
    int offset;
    /*位域成员使用：大小*/
    int bits;
    /*位域成员使用：偏移bit数*/
    int pos;
    Type field_type;
    struct field *next_field_ptr;
} *Field;

typedef struct recordType {
    TYPE_COMMON
    char *record_id;
    Field record_fields;
    Field *tail_field_ptr;
    /*是否包含constant field*/
    int hasConstField: 16;
    /*是否包含flexible array*/
    int hasFlexArray: 16;
    int isComplete;
} *RecordType;
```
> 《C编译器剖析》2.4   
C编译器对旧式风格的函数会进行一个被称为“实参提升”的动作，C编译器对变参函数中的无名参数也会进行实参提升。
- recordType的构造还有问题

## 0415日志
- 完成文件输出处理模块
    - 文件输出处理模块只有```LeftAlign```函数操作了其他的文件流，其他文件如何输出？
- 完成符号表管理模块
    - 对offset还是不熟悉 
- 完成论文第一章、第二章

## 0417日志
### 词法分析模块
- token.h包含了各种类型的token
    - 关键字 keywords
    - 操作符 operators
    - 标点符号 punctunators
    - 相比C标准多了
    ```c
    TOKEN(TK_END,           "EOF")
    ```
    - 相比C标准少了
    ```
    #
    ##
    ```
- ```setlocale```函数
> LC-CTYPE affects the behavior of the character handling functions and the multibyte functions.
## 0509日志
- 一个值得记录的问题  
对词法分析模块进行测试时发现```TokenStrings```中打印的符合token名字为乱码。
调用cmake自动生成的makefile的目标```make lex.c.i```发现预处理后的聚合初始化为空，该部分代码如下：
    ```c
    char *TokenStrings[] =
            {
    #define TOKEN(k, s) s,
    #include "token.h"
    #undef  TOKEN
            };
    ```
    发现问题在于CLion对于添加的头文件自动添加了防止重复包含的宏```SCC_TOKEN_H```，解决方法为在包含该头文件之前使用预处理指令```#undef SCC_TOKEN_H```或者删除token.h文件中防止重复包含的预处理指令。
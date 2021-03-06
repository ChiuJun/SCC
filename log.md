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
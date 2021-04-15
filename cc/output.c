//
// Created by LJChi on 2021/4/13.
//

#include "cc.h"

#define BUF_LEN 4096

char OutBuffer[BUF_LEN];
int BufferSize;

FILE *CreateOutput(char *filename, char *ext) {
    char tmp[256];
    char *ptr = tmp;

    while (*filename && *filename != '.')
        *ptr++ = *filename++;
    strcpy(ptr, ext);

    return fopen(tmp, "w");
}

/*清空buffer,向汇编文件写入数据*/
void Flush(void) {
    if (BufferSize != 0) {
        fwrite(OutBuffer, 1, BufferSize, ASMFile);
    }
    BufferSize = 0;
}

void PutChar(int ch) {
    if (BufferSize >= BUF_LEN) {
        Flush();
    }
    OutBuffer[BufferSize++] = ch;
}

void PutString(char *str) {
    int len = strlen(str);

    if (len > BUF_LEN) {
        if(BufferSize != 0)
            Flush();
        fwrite(str, 1, len, ASMFile);
        return;
    }
    if (len > BUF_LEN - BufferSize) {
        Flush();
    }
    memcpy(OutBuffer+BufferSize,str,len);
    BufferSize += len;
}

char *FormatName(const char *fmt, ...) {
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return IdentifierName(buf, strlen(buf));
}

void Print(const char *fmt, ...) {
    char tmp[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    PutString(tmp);
}

/*输出 @param pos 指定数量的空格*/
void LeftAlign(FILE *file, int pos) {
    char spaces[256];

    pos = pos >= 256 ? 2 : pos;
    memset(spaces, ' ', pos);
    spaces[pos] = '\0';
    if (file != ASMFile) {/*其他文件直接输出*/
        fprintf(file, "\n%s", spaces);
    } else {/*汇编文件放进buffer*/
        PutString(spaces);
    }
}

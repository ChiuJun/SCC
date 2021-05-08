//
// Created by LJChi on 2021/4/11.
//

#include "cc.h"

unsigned char END_OF_FILE = 255;
struct input Input;

/*
 * 读入预处理后的源文件至内存中
 * 在文件末尾加上 END_OF_FILE 标识文件结束
 * */
void ReadSourceFile(char *filename) {
    FILE *file_ptr = fopen(filename, "r");
    if (file_ptr == NULL) {
        Fatal("Can't open file: %s.", filename);
    }

    /*获取文件大小*/
    fseek(file_ptr, 0, SEEK_END);
    Input.file_size = ftell(file_ptr);

    Input.base = malloc(Input.file_size + 1);
    if (Input.base == NULL) {
        Fatal("The file is too big: %s.", filename);
        fclose(file_ptr);
    }
    fseek(file_ptr, 0, SEEK_SET);
    Input.file_size = fread(Input.base, 1, Input.file_size, file_ptr);
    fclose(file_ptr);

    Input.filename = (unsigned char *)filename;
    Input.base[Input.file_size] = END_OF_FILE;
    Input.cursor = Input.line_head = Input.base;
    Input.line = 1;
}

void CloseSourceFile(void){
    free(Input.base);
}
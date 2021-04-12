//
// Created by LJChi on 2021/4/11.
//

#ifndef SCC_INPUT_H
#define SCC_INPUT_H

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

extern unsigned char END_OF_FILE;
extern struct input Input;

void ReadSourceFile(char *filename);
void CloseSourceFile(void);

#endif //SCC_INPUT_H

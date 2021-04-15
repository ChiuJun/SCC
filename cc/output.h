//
// Created by LJChi on 2021/4/13.
//

#ifndef SCC_OUTPUT_H
#define SCC_OUTPUT_H

FILE* CreateOutput(char *filename, char *ext);
void Flush(void);
void PutChar(int ch);
void PutString(char *str);
char* FormatName(const char *fmt, ...);
void Print(const char *fmt, ...);
void LeftAlign(FILE *file, int pos);

#endif //SCC_OUTPUT_H

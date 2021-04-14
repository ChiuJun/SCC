//
// Created by LJChi on 2021/4/14.
//

#ifndef SCC_CONFIG_H
#define SCC_CONFIG_H

#undef  LITTLE_ENDIAN
#define LITTLE_ENDIAN     1

#define CHAR_SIZE         1
#define SHORT_SIZE        2
#define INT_SIZE          4
#define LONG_SIZE         4
#define LONG_LONG_SIZE    4
#define FLOAT_SIZE        4
#define DOUBLE_SIZE       8
#define LONG_DOUBLE_SIZE  8
#define POINTER_SIZE INT_SIZE

#define WCHAR		INT
#define	SCC_WCHAR_T	int

#endif //SCC_CONFIG_H

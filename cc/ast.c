//
// Created by LJChi on 2021/4/16.
//
#include "cc.h"

int CurrentToken;

const char *CurrentFileName;
int CurrentLineNum;

void DoExpect(int tok) {
    if (CurrentToken == tok) {
        NEXT_TOKEN;
        return;
    }

    fprintf(stderr, "%s,%d:", CurrentFileName, CurrentLineNum);
    if (tok == TK_SEMICOLON && TokenCoord.pp_line - PrevCoord.pp_line == 1) {
        DoError(&PrevCoord, "Expect ;");
    } else {
        DoError(&TokenCoord, "Expect %s", TokenStrings[tok - 1]);
    }
}

/*
 * 判断当前的token是否属于@tokens指定的集合中
 * @tokens必须是结尾为0的数组
 * */
int IsCurrentTokenIn(int tokens[]) {
    int *ptr = tokens;

    while (*ptr) {
        if (CurrentToken == *ptr)
            return 1;
        ptr++;
    }

    return 0;
}

/*
 * 跳过不在@tokens指定的token集合中的token
 * */
void DoSkipTo(int tokens[], char *errorMessage) {
    struct coord tmpCoord = TokenCoord;

    if (CurrentToken == TK_END || IsCurrentTokenIn(tokens))
        return;
    while (CurrentToken != TK_END) {
        NEXT_TOKEN;
        if (IsCurrentTokenIn(tokens))
            break;
    }
    fprintf(stderr, "%s,%d:", CurrentFileName, CurrentLineNum);
    DoError(&tmpCoord, "skip to %s\n", errorMessage);
}
//
// Created by LJChi on 2021/4/16.
//
#include <locale.h>
#include <wchar.h>
#include "cc.h"

#include "config.h"
#include "keyword.h"

#define CURSOR      (Input.cursor)
#define LINE        (Input.line)
#define LINE_HEAD    (Input.line_head)
#define IS_EOF(cur) (*(cur) == END_OF_FILE && ((cur)-Input.base) == Input.file_size)

typedef int (*Scanner)(void);

static unsigned char *PeekPoint;
static union value PeekValue;
static struct coord PeekCoord;
static Scanner Scanners[256];

union value TokenValue;
struct coord TokenCoord;
struct coord PrevCoord;

char *TokenStrings[] =
        {
#define TOKEN(k, s) s,

#include "token.h"

#undef  TOKEN
        };

/**
 * 处理预处理文件中的信息
 * file:demo.c
 * line1:   #include <stdio.h>
 * line2:
 * line3:   int a = 1;
 * line4:
 * line5:   main(){
 * line6:       printf("hello world!\n");
 * line7:   }
 * file:demo.i
 * line553: # 3 "demo.c"
 * line554: int a = 1;
 * line555:
 * line556: main(){
 * line557:     printf("hello world!\n");
 * line558: }
 * */
static void ScanPPLine(void) {
    int line = 0;

    CURSOR++;
    while (*CURSOR == ' ' || *CURSOR == '\t') {
        CURSOR++;
    }
    if (IsDigit(*CURSOR)) {
        /*将遇到的数值转化为int*/
        while (IsDigit(*CURSOR)) {
            line = 10 * line + (*CURSOR - '0');
            CURSOR++;
        }
        TokenCoord.src_line = line - 1;

        /*跳过空格获取文件名*/
        while (*CURSOR == ' ' || *CURSOR == '\t') {
            CURSOR++;
        }
        TokenCoord.src_filename = (char *)++CURSOR;
        while (*CURSOR != '"' && !IS_EOF(CURSOR) && *CURSOR != '\n') {
            CURSOR++;
        }
        TokenCoord.src_filename = IdentifierName((char *) TokenCoord.src_filename,
                                                                   CURSOR - (unsigned char *)TokenCoord.src_filename);
    }
    while (*CURSOR != '\n' && !IS_EOF(CURSOR)) {
        CURSOR++;
    }
}

static void SkipWhiteSpace(void) {
    unsigned int ch = *CURSOR;
    while (ch == '\t' || ch == '\v' || ch == '\f' || ch == ' ' ||
           ch == '\r' || ch == '\n' || ch == '/' || ch == '#') {
        switch (ch) {
            case '\n':
                TokenCoord.src_line++;
                LINE++;
                LINE_HEAD = ++CURSOR;
                break;
            case '#':
                /*
                 * 预处理文件的信息
                 * # 3 "demo.c"
                 * */
                ScanPPLine();
                break;
            case '/':
                /*
                 * 处理注释 实际上注释已经被预处理器删除
                 * 扩展了//风格的注释
                 * */
                if (CURSOR[1] != '/' && CURSOR[1] != '*')
                    return;
                CURSOR++;
                if (*CURSOR == '/') {
                    CURSOR++;
                    while (*CURSOR != '\n' && !IS_EOF(CURSOR)) {
                        CURSOR++;
                    }
                } else {
                    CURSOR += 1;
                    while (CURSOR[0] != '*' || CURSOR[1] != '/') {
                        if (*CURSOR == '\n') {
                            TokenCoord.src_line++;
                            LINE++;
                        } else if (IS_EOF(CURSOR) || IS_EOF(&CURSOR[1])) {
                            Error(&TokenCoord, "unterminated comment");
                            return;
                        }
                        CURSOR++;
                    }
                    CURSOR += 2;
                }
                break;

            default:
                CURSOR++;
                break;
        }
        ch = *CURSOR;
    }
}

/*
 * 处理转义字符
 * */
static int ScanEscapeChar(int wide) {
    int val = 0, overflow = 0;

    CURSOR++;
    switch (*CURSOR++) {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case '\'':
        case '"':
        case '\\':
        case '\?':
            return *(CURSOR - 1);
        case 'x':
            /* hexademical-espace-sequence */
            if (!IsHexDigit(*CURSOR)) {
                Error(&TokenCoord, "Expect hex digit");
                return 'x';
            }
            while (IsHexDigit(*CURSOR)) {
                if (val >> (WCharType->size * 8 - 4)) {
                    overflow = 1;
                }
                if (IsDigit(*CURSOR)) {
                    val = (val << 4) + *CURSOR - '0';
                } else {
                    val = (val << 4) + ToUpper(*CURSOR) - 'A' + 10;
                }
                CURSOR++;
            }
            if (overflow || (!wide && val > 255)) {
                Warning(&TokenCoord, "hexademical-espace-sequence overflow");
            }
            return val;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            /*
             * octal-escape-sequence
             *      octal-digit
             *      octal-digit octal-digit
             *      octal-digit octal-digit octal-digit
             */
            val = *(CURSOR - 1) - '0';
            if (IsOctDigit(*CURSOR)) {
                val = (val << 3) + *CURSOR++ - '0';
                if (IsOctDigit(*CURSOR))
                    val = (val << 3) + *CURSOR++ - '0';
            }
            return val;
        default:
            Warning(&TokenCoord, "Unrecognized escape sequence:\\%c", *CURSOR);
            return *CURSOR;
    }
}

/*
 * 扫描字符字面量
 * 所有字符都作为宽字符保存
 * */
static int ScanCharLiteral(void) {
    SCC_WCHAR_T ch = 0;
    size_t n = 0;
    int count = 0;
    int wide = 0;

    if (*CURSOR == 'L') {
        CURSOR++;
        wide = 1;
    }
    CURSOR++;
    if (*CURSOR == '\'') {
        Error(&TokenCoord, "empty character constant");
    } else if (*CURSOR == '\n' || IS_EOF(CURSOR)) {
        Error(&TokenCoord, "missing terminating ' character");
    } else {
        if (*CURSOR == '\\') {
            ch = (SCC_WCHAR_T) ScanEscapeChar(wide);
        } else {
            if (wide) {
                n = mbrtowc((wchar_t *) &ch, (char *) CURSOR, MB_CUR_MAX, 0);
                if (n > 0) {
                    CURSOR += n;
                }
            } else {
                ch = *CURSOR;
                CURSOR++;
            }
        }
        while (*CURSOR != '\'') {
            if (*CURSOR == '\n' || IS_EOF(CURSOR))
                break;
            CURSOR++;
            count++;
        }
    }
    TokenValue.i[1] = 0;
    TokenValue.i[0] = ch;

    if (*CURSOR != '\'') {
        Error(&TokenCoord, "missing terminating ' character");
    } else {
        CURSOR++;
        if (count != 0) {
            Warning(&TokenCoord, "Too many characters");
        }
    }

    return TK_INTCONST;
}

/*
 * 扫描字符串字面量
 *
 * */
static int ScanStringLiteral(void) {
    char tmp[512];
    char *cp = tmp;
    SCC_WCHAR_T *wcp = (SCC_WCHAR_T *) tmp;
    int wide = 0;
    int len = 0;
    int maxLen = 512;
    SCC_WCHAR_T ch = 0;
    String str;
    size_t n = 0;
    CALLOC(str);

    if (*CURSOR == 'L') {
        CURSOR++;
        wide = 1;
        maxLen /= sizeof(SCC_WCHAR_T);
    }

    CURSOR++;
    next_string:
    while (*CURSOR != '"') {
        if (*CURSOR == '\n' || IS_EOF(CURSOR))
            break;
        if (*CURSOR == '\\') {
            ch = (SCC_WCHAR_T) ScanEscapeChar(wide);
        } else {
            if (wide) {
                n = mbrtowc((wchar_t *) &ch, (char *) CURSOR, MB_CUR_MAX, 0);
                if (n > 0) {
                    CURSOR += n;
                } else {
                    ch = *CURSOR;
                    CURSOR++;
                }
            } else {
                ch = *CURSOR;
                CURSOR++;
            }
        }
        if (wide) {
            wcp[len] = ch;
        } else {
            cp[len] = (char) ch;
        }
        len++;
        if (len >= maxLen) {
            AppendSTR(str, tmp, len, wide);
            len = 0;
        }
    }

    if (*CURSOR != '"') {
        Error(&TokenCoord, "Expect \"");
        goto end_string;
    }

    CURSOR++;
    SkipWhiteSpace();
    if (CURSOR[0] == '"') {
        if (wide == 1) { /*未定义行为，这里给予报错*/
            Error(&TokenCoord, "String wideness mismatch");
        }
        CURSOR++;
        goto next_string;
    } else if (CURSOR[0] == 'L' && CURSOR[1] == '"') {
        if (wide == 0) { /*未定义行为，这里给予报错*/
            Error(&TokenCoord, "String wideness mismatch");
        }
        CURSOR += 2;
        goto next_string;
    }

    end_string:
    AppendSTR(str, tmp, len, wide);
    TokenValue.p = str;

    return wide ? TK_WIDESTRING : TK_STRING;
}

/*
 * 尝试从 struct keyword *keywords[] 表中匹配关键字
 * 返回代表对应关键字或者标识符的token
 * */
static int FindKeyword(char *str, int len) {
    struct keyword *p = NULL;
    int index = ToUpper(*str) - 'A';
    if(*str == '_')
        return TK_ID;

    p = keywords[index];
    while (p->name) {
        if (p->len == len && strncmp(str, p->name, len) == 0)
            break;
        p++;
    }

    return p->tok;
}

/*处理字母、下划线开头的token*/
static int ScanIdentifier(void) {
    unsigned char *start = CURSOR;
    int tok;

    if (*CURSOR == 'L') {/*宽字符字面量*/
        if (CURSOR[1] == '\'') {/*L'c'*/
            return ScanCharLiteral();
        } else if (CURSOR[1] == '"') {/*L"string"*/
            return ScanStringLiteral();
        }
    }

    /*字母数字下划线*/
    while (IsLetterOrDigit(*CURSOR)) {
        CURSOR++;
    }
    tok = FindKeyword((char *) start, (int) (CURSOR - start));
    if (tok == TK_ID) {
        /*对于标识符保持唯一的拷贝*/
        TokenValue.p = IdentifierName((char *) start, (int) (CURSOR - start));
    }

    return tok;
}

/*
 * Integer constants 的识别
 * */
static int ScanIntLiteral(unsigned char *start, int len, int base) {
    unsigned char *p = start;
    unsigned char *end = start + len;
    unsigned int i[2] = {0, 0};
    unsigned int t1, t2;
    int carry0 = 0, carry1 = 0;
    int overflow = 0, d = 0;
    int tok;

    while (p != end) {
        if ((*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')) {
            d = ToUpper(*p) - 'A' + 10;
        } else {
            d = *p - '0';
        }
        /*
         * i[1]高32位，i[0]低32位
         * */
        switch (base) {
            case 16:
                carry0 = HIGH_4BIT(i[0]);
                carry1 = HIGH_4BIT(i[1]);
                i[0] = i[0] << 4;
                i[1] = i[1] << 4;
                break;
            case 8:
                carry0 = HIGH_3BIT(i[0]);
                carry1 = HIGH_3BIT(i[1]);
                i[0] = i[0] << 3;
                i[1] = i[1] << 3;
                break;
            case 10:
                carry0 = HIGH_3BIT(i[0]) + HIGH_1BIT(i[0]);
                carry1 = HIGH_3BIT(i[1]) + HIGH_1BIT(i[1]);

                t1 = i[0] << 3;
                t2 = i[0] << 1;
                if (t1 > UINT_MAX - t2) {
                    carry0++;
                }
                i[0] = t1 + t2;

                t1 = i[1] << 3;
                t2 = i[1] << 1;
                if (t1 > UINT_MAX - t2) {
                    carry1++;
                }
                i[1] = t1 + t2;

                break;
        }
        if (i[0] > UINT_MAX - d) {
            carry0 += i[0] - (UINT_MAX - d);
        }
        if (carry1 || (i[1] > UINT_MAX - carry0)) {
            overflow = 1;
        }
        i[0] += d;
        i[1] += carry0;
        p++;
    }

    tok = TK_INTCONST;
    /*
     * 123U 123u
     * */
    if (*CURSOR == 'U' || *CURSOR == 'u') {
        CURSOR++;
        tok = TK_UINTCONST;
    }
    /*
     * L、l、LL、ll
     * */
    if (*CURSOR == 'L' || *CURSOR == 'l') {
        CURSOR++;
        if (tok == TK_INTCONST) {
            tok = TK_LONGCONST;
        } else if (tok == TK_UINTCONST) {
            tok = TK_ULONGCONST;
        }
        if (*CURSOR == 'L' || *CURSOR == 'l') {
            CURSOR++;
            if (tok == TK_ULONGCONST) {
                tok = TK_ULLONGCONST;
            } else {
                tok = TK_LLONGCONST;
            }
        }
    }

    /*
     * overflow 不为0，64位整数溢出
     * i[1] 不为0，32位整数溢出
     * 简单起见，所有整数均为32位
     * */
    if (overflow || i[1] != 0) {
        Warning(&TokenCoord, "Integer literal is too big");
    }
    TokenValue.i[1] = 0;
    TokenValue.i[0] = i[0];

    return tok;
}

/*
 * Floating constants 的识别
 * 调用库函数 strtod 完成
 * */
static int ScanFloatLiteral(unsigned char *start) {
    double d;

    if (*CURSOR == '.') {
        CURSOR++;
        while (IsDigit(*CURSOR)) {
            CURSOR++;
        }
    }
    if (*CURSOR == 'e' || *CURSOR == 'E') {
        CURSOR++;
        if (*CURSOR == '+' || *CURSOR == '-') {
            CURSOR++;
        }
        if (!IsDigit(*CURSOR)) {
            Error(&TokenCoord, "Expect exponent value");
        } else {
            while (IsDigit(*CURSOR)) {
                CURSOR++;
            }
        }
    }

    errno = 0;
    d = strtod((char *) start, NULL);
    if (errno == ERANGE) {
        Warning(&TokenCoord, "Float literal overflow");
    }

    TokenValue.d = d;
    if (*CURSOR == 'f' || *CURSOR == 'F') {
        CURSOR++;
        TokenValue.f = (float) d;
        return TK_FLOATCONST;
    } else if (*CURSOR == 'L' || *CURSOR == 'l') {
        CURSOR++;
        return TK_LDOUBLECONST;
    } else {
        return TK_DOUBLECONST;
    }
}

/*
 * 扫描数值字面量
 * '.'开头的数值字面量(eg: .123 )在ScanDot()中作了处理
 * */
static int ScanNumericLiteral(void) {
    unsigned char *start = CURSOR;
    int base;

    if (*CURSOR == '0' && (CURSOR[1] == 'x' || CURSOR[1] == 'X')) {/* 0x123 或者 0X123 */
        CURSOR += 2;
        start = CURSOR;
        base = 16;
        if (!IsHexDigit(*CURSOR)) {
            Error(&TokenCoord, "Expect hex digit");
            TokenValue.i[0] = 0;
            return TK_INTCONST;
        }
        while (IsHexDigit(*CURSOR)) {
            CURSOR++;
        }
    } else if (*CURSOR == '0') { /*'0'开头，8进制数*/
        CURSOR++;
        base = 8;
        while (IsOctDigit(*CURSOR)) {
            CURSOR++;
        }
    } else { /*10进制数*/
        CURSOR++;
        base = 10;
        while (IsDigit(*CURSOR)) {
            CURSOR++;
        }
    }

    if (base == 16 || *CURSOR != '.') {
        return ScanIntLiteral(start, (int) (CURSOR - start), base);
    } else {
        return ScanFloatLiteral(start);
    }

}

static int ScanPlus(void) {
    switch (*++CURSOR) {
        case '=':/* += */
            CURSOR++;
            return TK_ADD_ASSIGN;
        case '+':/* ++ */
            CURSOR++;
            return TK_INC;
        default:/* + */
            return TK_ADD;
    }
}

static int ScanMinus(void) {
    switch (*++CURSOR) {
        case '=':/* -= */
            CURSOR++;
            return TK_SUB_ASSIGN;
        case '-':/* -- */
            CURSOR++;
            return TK_DEC;
        case '>':/* -> */
            CURSOR++;
            return TK_POINTER;
        default:/* - */
            return TK_SUB;
    }
}

static int ScanStar(void) {
    switch (*++CURSOR) {
        case '=':/* *= */
            CURSOR++;
            return TK_MUL_ASSIGN;
        default:/* * */
            return TK_MUL;
    }
}

static int ScanSlash(void) {
    switch (*++CURSOR) {
        case '=':/* /= */
            CURSOR++;
            return TK_DIV_ASSIGN;
        default:/* / */
            return TK_DIV;
    }
}

static int ScanPercent(void) {
    switch (*++CURSOR) {
        case '=':/* %= */
            CURSOR++;
            return TK_MOD_ASSIGN;
        default:/* % */
            return TK_MOD;
    }
}

static int ScanLess(void) {
    switch (*++CURSOR) {
        case '=':/* <= */
            CURSOR++;
            return TK_LESS_EQ;
        case '<':
            CURSOR++;
            if (*CURSOR == '=') {/* <<= */
                CURSOR++;
                return TK_LSHIFT_ASSIGN;
            } else {/* << */
                return TK_LSHIFT;
            }
        default:/* < */
            return TK_LESS;
    }
}

static int ScanGreat(void) {
    switch (*++CURSOR) {
        case '=':/* >= */
            CURSOR++;
            return TK_GREAT_EQ;
        case '>':
            CURSOR++;
            if (*CURSOR == '=') {/* >>= */
                CURSOR++;
                return TK_RSHIFT_ASSIGN;
            } else {/* >> */
                return TK_RSHIFT;
            }
        default:/* > */
            return TK_GREAT;
    }
}

static int ScanExclamation(void) {
    switch (*++CURSOR) {
        case '=':/* != */
            CURSOR++;
            return TK_UNEQUAL;
        default:/* ! */
            return TK_NOT;
    }
}

static int ScanEqual(void) {
    switch (*++CURSOR) {
        case '=':/* == */
            CURSOR++;
            return TK_EQUAL;
        default:/* = */
            return TK_ASSIGN;
    }
}

static int ScanBar(void) {
    switch (*++CURSOR) {
        case '=':/* |= */
            CURSOR++;
            return TK_BITOR_ASSIGN;
        case '|':/* || */
            CURSOR++;
            return TK_OR;
        default:/* | */
            return TK_BITOR;
    }
}

static int ScanAmpersand(void) {
    switch (*++CURSOR) {
        case '=':/* &= */
            CURSOR++;
            return TK_BITAND_ASSIGN;
        case '&':/* && */
            CURSOR++;
            return TK_AND;
        default:/* & */
            return TK_BITAND;
    }
}

static int ScanCaret(void) {
    switch (*++CURSOR) {
        case '=':/* ^= */
            CURSOR++;
            return TK_BITXOR_ASSIGN;
        default:/* ^ */
            return TK_BITXOR;
    }
}

static int ScanDot(void) {
    if (IsDigit(CURSOR[1])) {/*'.'开头的数值字面量，eg: .123 */
        return ScanFloatLiteral(CURSOR);
    } else if (CURSOR[1] == '.' && CURSOR[2] == '.') {
        CURSOR += 3;
        return TK_ELLIPSIS;
    } else {
        CURSOR++;
        return TK_DOT;
    }
}

/*
 * 无法识别的非法字符
 * */
static int ScanBadChar(void) {
    Error(&TokenCoord, "illegal character:0x%x", *CURSOR);
    CURSOR++;
    return GetNextToken();
}

/*
 * 扫描单字符的token
 * */
static int ScanSingleCharToken(void) {
    switch (*CURSOR++) {
        case '{':
            return TK_LBRACE;
        case '}':
            return TK_RBRACE;
        case '[':
            return TK_LBRACKET;
        case ']':
            return TK_RBRACKET;
        case '(':
            return TK_LPAREN;
        case ')':
            return TK_RPAREN;
        case ',':
            return TK_COMMA;
        case ';':
            return TK_SEMICOLON;
        case '~':
            return TK_COMP;
        case '?':
            return TK_QUESTION;
        case ':':
            return TK_COLON;
        default:
            --CURSOR;
            return ScanBadChar();
    }
}

/*
 * 文件结束符，input.c 追加的一个值为 END_OF_FILE 的符号
 * */
static int ScanEOF(void) {
    if (!IS_EOF(CURSOR)) {
        return ScanBadChar();
    }
    return TK_END;
}

void SetupLexer(void) {
    int idx;

    setlocale(LC_CTYPE, "");

    for (idx = 0; idx < END_OF_FILE + 1; idx++) {
        if (IsLetter(idx)) {/*字母和下划线：a-z A-Z _*/
            Scanners[idx] = ScanIdentifier;
        } else if (IsDigit(idx)) {
            Scanners[idx] = ScanNumericLiteral;
        } else {
            Scanners[idx] = ScanBadChar;
        }
    }
    Scanners[END_OF_FILE] = ScanEOF;

    Scanners['\''] = ScanCharLiteral;
    Scanners['"'] = ScanStringLiteral;

    Scanners['+'] = ScanPlus;
    Scanners['-'] = ScanMinus;
    Scanners['*'] = ScanStar;
    Scanners['/'] = ScanSlash;
    Scanners['%'] = ScanPercent;
    Scanners['<'] = ScanLess;
    Scanners['>'] = ScanGreat;
    Scanners['!'] = ScanExclamation;
    Scanners['='] = ScanEqual;
    Scanners['|'] = ScanBar;
    Scanners['&'] = ScanAmpersand;
    Scanners['^'] = ScanCaret;
    Scanners['.'] = ScanDot;

    Scanners['{'] = ScanSingleCharToken;
    Scanners['}'] = ScanSingleCharToken;
    Scanners['['] = ScanSingleCharToken;
    Scanners[']'] = ScanSingleCharToken;
    Scanners['('] = ScanSingleCharToken;
    Scanners[')'] = ScanSingleCharToken;
    Scanners[','] = ScanSingleCharToken;
    Scanners[';'] = ScanSingleCharToken;
    Scanners['~'] = ScanSingleCharToken;
    Scanners['?'] = ScanSingleCharToken;
    Scanners[':'] = ScanSingleCharToken;
}

/*
 * 获取下一个token:
 * 跳过空白符，更新TokenCoord的信息,调用对应的函数处理
 * src_line 与 src_filename 的信息会在 SkipWhiteSpace() 处理#时候完成
 * */
int GetNextToken(void) {

    PrevCoord = TokenCoord;
    SkipWhiteSpace();
    TokenCoord.pp_line = LINE;
    TokenCoord.col = (int) (CURSOR - LINE_HEAD + 1);

    return (*Scanners[*CURSOR])();
}

/*
 * 标记当前符号信息
 * 用于回溯
 * */
void BeginPeekToken(void) {
    PeekPoint = CURSOR;
    PeekValue = TokenValue;
    PeekCoord = TokenCoord;
}

/*
 * 重置当前符号信息
 * 用于回溯
 * */
void EndPeekToken(void) {
    CURSOR = PeekPoint;
    TokenValue = PeekValue;
    TokenCoord = PeekCoord;
}

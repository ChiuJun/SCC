//
// Created by LJChi on 2021/5/23.
//

#include "cc.h"
#include "decl.h"
#include "expr.h"
#include "gen.h"
#include "output.h"
#include "target.h"


#include "config.h"

int SwitchTableNum;

/**
 * Emit all the strings to assembly file
 */
static void EmitStrings(void) {
    Symbol p = Strings;
    String str;
    int len;
    int size;

    while (p) {
        DefineGlobal(p);
        str = p->val.p;
        //assert(p->ty->categ == ARRAY);
        len = strlen(p->access_name);
        size = str->len + 1;
        if (p->symbol_type->base_type == WCharType) {
            int i = 0;
            union value val;
            SCC_WCHAR_T *wcp = (SCC_WCHAR_T *) str->content;

            val.i[1] = 0;
            while (i < size) {
                val.i[0] = wcp[i];
                DefineValue(WCharType, val);
                LeftAlign(ASMFile, len);
                PutString("\t");
                i++;
            }
            PutString("\n");
        } else {
            DefineString(str, size);
        }
        p = p->next;
    }
    PutString("\n");
}

void EmitFloatConstants(void) {
    Symbol p = FloatConstants;

    while (p) {
        DefineFloatConstant(p);
        p = p->next;
    }
    PutString("\n");
}

static void EmitGlobals(void) {
    Symbol p = Globals;
    InitData initd;
    int len, size;


    int changed = 0;

    while (p) {
        initd = AsVar(p)->data;
        /**
        added for empty struct or array of empty struct,
            we can change the size to 1 here .
         */
        changed = 0;
        if (p->symbol_type->size == 0) {
            changed = 1;
            //PRINT_DEBUG_INFO(("%s",p->name));
            p->symbol_type->size = EMPTY_OBJECT_SIZE;
            p->symbol_type->align = EMPTY_OBJECT_ALIGN;
        }

        if (p->storage_class == TK_EXTERN && initd == NULL) {

        } else if (initd == NULL) {
            DefineCommData(p);
        } else {
            DefineGlobal(p);
            len = strlen(p->access_name);
            size = 0;
            while (initd) {
                //PRINT_DEBUG_INFO(("offset = %d ,size =  %d, val = %d",initd->offset,size, initd->expr->val.i[0]));
                if (initd->offset != size) {
                    LeftAlign(ASMFile, len);
                    PutString("\t");
                    Space(initd->offset - size);
                }
                if (initd->offset != 0) {
                    LeftAlign(ASMFile, len);
                    PutString("\t");
                }
                if (initd->expr->op == OP_ADD) {
                    int n = initd->expr->kids[1]->exprValue.i[0];

                    DefineAddress(initd->expr->kids[0]->exprValue.p);
                    if (n != 0) {
                        Print("%s%d", n > 0 ? " + " : " ", n);
                    }
                    PutString("\n");
                } else if (initd->expr->op == OP_STR) {
                    String str = initd->expr->exprValue.p;
                    size = initd->expr->ty->size / initd->expr->ty->base_type->size;
                    if (initd->expr->ty->base_type == WCharType) {
                        int i = 0;
                        union value val;
                        SCC_WCHAR_T *wcp = (SCC_WCHAR_T *) str->content;

                        val.i[1] = 0;
                        while (i < size) {
                            val.i[0] = wcp[i];
                            DefineValue(WCharType, val);
                            LeftAlign(ASMFile, len);
                            PutString("\t");
                            i++;
                        }
                    } else {
                        DefineString(str, size);
                    }
                } else {
                    DefineValue(initd->expr->ty, initd->expr->exprValue);
                }
                size = initd->offset + initd->expr->ty->size;
                initd = initd->next;
            }
            if (size < p->symbol_type->size) {
                LeftAlign(ASMFile, len);
                PutString("\t");
                Space(p->symbol_type->size - size);
            }
            PutString("\n");
        }
        // restore the zero size , though it is not needed here.
        if (changed && p->symbol_type->size == EMPTY_OBJECT_SIZE) {
            p->symbol_type->size = 0;
            changed = 0;
        }
        p = p->next;
    }
    PutString("\n");
}

/**
 * Emit all the functions
 */
static void EmitFunctions(AstTranslationUnit transUnit) {
    AstNode p;
    FunctionSymbol fsym;

    p = transUnit->externalDeclarations;
    while (p != NULL) {
        if (p->astNodeKind == NK_Function) {
            fsym = ((AstFunction) p)->funSymbol;
            if (fsym->storage_class != TK_STATIC || fsym->ref > 0) {
                EmitFunction(fsym);
            }
        }
        p = p->next;
    }
}

/**
 * Emit  assembly code for the translation unit
 */
void EmitTranslationUnit(AstTranslationUnit transUnit) {
    if (ASMFileName) {
        ASMFile = fopen(ASMFileName, "w");
        ASMFileName = NULL;
    } else {
        ASMFile = CreateOutput((char *)Input.filename, ASMExtName);
    }
    SwitchTableNum = 1;
    // "# Code auto-generated by SCC\n\n"
    BeginProgram();
    // ".data\n\n"
    Segment(DATA);
    /**
        .str0:	.string	"%d \012"
        .str1:	.string	"a + b + c + d = %d.\012"
     */
    EmitStrings();

    EmitFloatConstants();

    EmitGlobals();
    // ".text\n\n"
    Segment(CODE);

    /**
        The key function is
            void EmitFunction(FunctionSymbol fsym)
        in x86.c
     */
    EmitFunctions(transUnit);

    EndProgram();

    fclose(ASMFile);
}

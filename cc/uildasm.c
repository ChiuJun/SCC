//
// Created by LJChi on 2021/5/23.
//

#include "cc.h"
#include "decl.h"
#include "gen.h"

#define DST  inst->opds[0]
#define SRC1 inst->opds[1]
#define SRC2 inst->opds[2]

static char *OPCodeNames[] =
        {
#define OPRCODE(code, name, func) name,

#include "oprcode.h"

#undef OPRCODE
        };

char *GetUilCodeName(int code) {
    return OPCodeNames[code];
}

/**
	Program --> IRInst *
	IRInst --> AssignInst | BranchInst | JumpInst | IJumpInst
				| ReturnInst | CallInst | ClearInst
 */
static void DisassemblyUIL(IRInst inst) {
    int op = inst->opcode;

    fprintf(IRFile, "\t");
    switch (op) {
        case BOR:
        case BXOR:
        case BAND:
        case LSH:
        case RSH:
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case MOD:
            fprintf(IRFile, "%s : %s %s %s", DST->name, SRC1->name, OPCodeNames[op], SRC2->name);
            break;

        case INC:
        case DEC:
            fprintf(IRFile, "%s%s", OPCodeNames[op], DST->name);
            break;

        case BCOM:
        case NEG:
        case ADDR:
        case DEREF:
            fprintf(IRFile, "%s :%s%s", DST->name, OPCodeNames[op], SRC1->name);
            break;

        case MOV:
            // d = 4;
            fprintf(IRFile, "%s = %s", DST->name, SRC1->name);
            break;

        case IMOV:    // indirect mov
            fprintf(IRFile, "*%s = %s", DST->name, SRC1->name);
            break;

        case JE:
        case JNE:
        case JG:
        case JL:
        case JGE:
        case JLE:
            // 	if (n > 2) goto BB0;
            fprintf(IRFile, "if (%s %s %s) goto %s", SRC1->name, OPCodeNames[op],
                    SRC2->name, ((BBlock) DST)->sym->name);
            break;

        case JZ:
            fprintf(IRFile, "if (! %s) goto %s", SRC1->name, ((BBlock) DST)->sym->name);
            break;

        case JNZ:
            fprintf(IRFile, "if (%s) goto %s", SRC1->name, ((BBlock) DST)->sym->name);
            break;

        case JMP:
            // 	goto BB3;
            fprintf(IRFile, "goto %s", ((BBlock) DST)->sym->name);
            break;

        case IJMP:    // indirect jmp
            // goto(label1,label2,...)[VarName]
        {
            BBlock *p = (BBlock *) DST;

            fprintf(IRFile, "goto (");
            while (*p != NULL) {
                fprintf(IRFile, "%s,", (*p)->sym->name);
                p++;
            }
            fprintf(IRFile, ")[%s]", SRC1->name);
        }
            break;

        case CALL:
            // [VarName=] call VarName(Operand,Operand,...)
        {
            ILArg arg;
            Vector args = (Vector) SRC2;
            int i;
            // t1 = f(t0);
            if (DST != NULL) {
                fprintf(IRFile, "%s : ", DST->name);
            }
            //	printf(t0, t1);
            fprintf(IRFile, "%s(", SRC1->name);
            for (i = 0; i < GET_VECTOR_SIZE(args); ++i) {
                arg = GET_VECTOR_ITEM(args, i);
                if (i != GET_VECTOR_SIZE(args) - 1)
                    fprintf(IRFile, "%s, ", arg->sym->name);
                else
                    fprintf(IRFile, "%s", arg->sym->name);
            }
            fprintf(IRFile, ")");
        }
            break;

        case RET:
            // return t4;
            fprintf(IRFile, "return %s", DST->name);
            break;

        default:
            fprintf(IRFile, "%s : %s%s", DST->name, OPCodeNames[op], SRC1->name);
            break;
    }

    fprintf(IRFile, ";\n");


}

void DisassemblyFunction(AstFunction func) {
    FunctionSymbol fsym = func->funSymbol;
    BBlock bb = fsym->entryBB;
    IRInst inst;

    if (!fsym->defined)
        return;
    /**
        function f
            if (n > 2) goto BB0;
            return 1;
            goto BB1;
            goto BB1;
        BB0:
            t0 = n + -1;
            t1 = f(t0);
            t2 = n + -2;
            t3 = f(t2);
            t4 = t1 + t3;
            return t4;
        BB1:
            ret
     */
    fprintf(IRFile, "function %s\n", fsym->name);

    while (bb != NULL) {
        //	BB0:
        if (bb->sym && bb->npred > 0) {
#if 0    // commented
            fprintf(IRFile, "\n%s: ref = %d, sym->ref = %d ,npred = %d , nsucc = %d \n",
                bb->sym->name, bb->ref, bb->sym->ref,bb->npred, bb->nsucc);
#endif
#if 1
            fprintf(IRFile, "%s:\n ", bb->sym->name);
#endif
        }
        inst = bb->insth.next;
        while (inst != &bb->insth) {
            DisassemblyUIL(inst);
            inst = inst->next;
        }
        bb = bb->next;
    }
    // if the exit block has predecessors, gen a  "ret" intermediate instruction
    if (fsym->exitBB->npred != 0)
        fprintf(IRFile, "\tret\n");

    fprintf(IRFile, "\n\n");
}

void DisassemblyTranslationUnit(AstTranslationUnit transUnit) {
    AstNode p = transUnit->externalDeclarations;

    IRFile = CreateOutput((char *)Input.filename, ".uil");

    while (p) {
        if (p->astNodeKind == NK_Function) {
            DisassemblyFunction((AstFunction) p);
        }
        p = p->next;
    }

    fclose(IRFile);
}

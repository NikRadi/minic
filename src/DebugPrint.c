#include "DebugPrint.h"
#include "ErrorPrint.h"


static void PrintAst(Ast *ast, int indent);
static void PrintBlock(Block *block, int indent);


static void PrintIndent(int indent, char *format, ...) {
    va_list arglist;
    va_start(arglist, format);
    for (int i = 0; i < indent; ++i) {
        printf(" ");
    }

    vfprintf(stdout, format, arglist);
}

static void PrintFuncDecl(FuncDecl *funcdecl, int indent) {
    PrintIndent(indent,
        "<FuncDecl ident=\"%s\" stack_depth_bytes=\"%d\" returntype=\"%d\">\n",
        funcdecl->ident,
        funcdecl->stack_depth_bytes,
        funcdecl->returntype
    );

    Node2Links *node = funcdecl->params.head;
    for (int i = 0; i < funcdecl->params.count; ++i) {
        PrintAst(node->item, indent + 4);
        node = node->next;
    }

    PrintBlock(funcdecl->block, indent + 4);
    PrintIndent(indent, "<FuncDecl/>\n");
}

static void PrintBlock(Block *block, int indent) {
    PrintIndent(indent, "<Block>\n");
    for (int i = 0; i < block->stmts.count; ++i) {
        // PrintAst((Ast *) ListGet(&block->stmts, i), indent + 4);
    }

    PrintIndent(indent, "<Block/>\n");
}

static void PrintVarDecl(VarDecl *vardecl, int indent) {
    PrintIndent(indent,
        "<VarDecl ident=\"%s\" datatype=\"%d\" arrsize=\"%d\" lvl_indirection=\"%d\"",
        vardecl->ident,
        vardecl->datatype,
        vardecl->arrsize,
        vardecl->lvl_indirection
    );

    if (vardecl->expr != NULL) {
        printf(">\n");
        PrintAst(vardecl->expr, indent + 4);
        PrintIndent(indent, "<VarDecl/>\n");
    }
    else {
        printf("/>\n");
    }
}

static void PrintBinaryOp(BinaryOp *binaryop, int indent) {
    PrintIndent(indent,
        "<BinaryOp optype=\"%s\">\n",
        GetOperatorTypeStr(binaryop->optype)
    );

    PrintAst(binaryop->lhs, indent + 4);
    PrintAst(binaryop->rhs, indent + 4);
    PrintIndent(indent, "<BinaryOp/>\n");
}

static void PrintUnaryOp(UnaryOp *unaryop, int indent) {
    PrintIndent(indent,
        "<UnaryOp optype=\"%s\">\n",
        GetOperatorTypeStr(unaryop->optype)
    );

    PrintAst(unaryop->expr, indent + 4);
    PrintIndent(indent, "<UnaryOp/>\n");
}

static void PrintFuncCall(FuncCall *vardecl, int indent) {
    PrintIndent(indent, "<FuncCall...>\n");
}

static void PrintLiteralIdentInt(Literal *literal, int indent) {
    PrintIndent(indent,
        "<Literal intvalue=\"%d\"/>\n",
        literal->intvalue
    );
}

static void PrintLiteralIdentStr(Literal *literal, int indent) {
    PrintIndent(indent,
        "<Literal strvalue=\"%s\">\n",
        literal->strvalue
    );
}

static void PrintAst(Ast *ast, int indent) {
    switch (ast->type) {
        case AST_FUNCDECL:      {PrintFuncDecl((FuncDecl *) ast, indent);} break;
        case AST_BLOCK:         {PrintBlock((Block *) ast, indent);} break;
        case AST_VARDECL:       {PrintVarDecl((VarDecl *) ast, indent);} break;
        case AST_UNARYOP:       {PrintUnaryOp((UnaryOp *) ast, indent);} break;
        case AST_BINARYOP:      {PrintBinaryOp((BinaryOp *) ast, indent);} break;
        case AST_FUNCCALL:      {PrintFuncCall((FuncCall *) ast, indent);} break;
        case AST_LITERAL_IDENT: {PrintLiteralIdentStr((Literal *) ast, indent);} break;
        case AST_LITERAL_INT:   {PrintLiteralIdentInt((Literal *) ast, indent);} break;
        default: {
            ThrowInternalError("not implemented '%s'\n", GetAstTypeStr(ast->type));
        }
    }
}

char *GetTokenTypeStr(TokenType type) {
    switch (type) {
#define TOKEN(name, str) case TOKEN_##name: return #str;
#include "TokenTypes.def"
        default: {
            return NULL;
        }
    }
}

char *GetAstTypeStr(AstType type) {
    switch (type) {
#define AST(name, str) case AST_##name: return #str;
#include "AstNodes.def"
        default: {
            return NULL;
        }
    }
}

char *GetOperatorTypeStr(OperatorType type) {
    switch (type) {
#define OP(name) case name: return #name;
#include "AstNodes.def"
        default: {
            return NULL;
        }
    }
}

void PrintToken(Token token) {
    switch (token.type) {
#define TOKEN(name, str) case TOKEN_##name: {\
        printf("<Token line=\"%d\" type=\"%s\"/>", token.line, #name);\
    } break;
#include "TokenTypes.def"
        default: {
            ThrowInternalError("unknown TokenType '%d'", token.type);
        }
    }
}

void PrintFile(File *file) {
    printf("<File name=\"%s\">\n", file->name);
    Node2Links *node = file->decls.head;
    for (int i = 0; i < file->decls.count; ++i) {
        PrintAst(node->item, 4);
        node = node->next;
    }

    printf("<File/>\n");
}
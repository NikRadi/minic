#include "Typechecking.h"


static void TypecheckBlock(Block *block);
static void TypecheckExpr(Ast *expr);


static char *func_idents[50];
static int num_funcs = 0;


static void TypecheckLiteral(Literal *literal) {

}

static void TypecheckBinaryOp(BinaryOp *binaryop) {
    TypecheckExpr(binaryop->lhs);
    TypecheckExpr(binaryop->rhs);
}

static void TypecheckExpr(Ast *expr) {
    switch (expr->type) {
        case AST_INT_LITERAL: {TypecheckLiteral((Literal *) expr);} break;
        case AST_BINARYOP:    {TypecheckBinaryOp((BinaryOp *) expr);} break;
        default: {
            printf("is this even possible\n");
            exit(1);
        };
    }
}

static void TypecheckVarDecl(VarDecl *vardecl) {
    if (vardecl->expr != 0) {
        TypecheckExpr(vardecl->expr);
    }
}

static void TypecheckVarAssign(VarAssign *varassign) {
    TypecheckExpr(varassign->expr);
}

static void TypecheckIfStmt(IfStmt *ifstmt) {
    TypecheckExpr(ifstmt->condition);
    // TypecheckBlock(ifstmt->if_block);
}

static void TypecheckWhileLoop(WhileLoop *whileloop) {
    TypecheckExpr(whileloop->condition);
    TypecheckBlock(whileloop->block);
}

static void TypecheckForLoop(ForLoop *forloop) {
    TypecheckVarDecl(forloop->pre_operation);
    TypecheckExpr(forloop->condition);
    TypecheckVarAssign(forloop->post_operation);
    TypecheckBlock(forloop->block);
}

static void TypecheckBlock(Block *block) {
    Block *child_block = block;
    while (child_block != 0 && child_block->stmt != 0) {
        switch (child_block->stmt->type) {
            case AST_VARDECL:   {TypecheckVarDecl((VarDecl *) child_block->stmt);} break;
            case AST_VARASSIGN: {TypecheckVarAssign((VarAssign *)child_block->stmt);} break;
            case AST_IFSTMT:    {TypecheckIfStmt((IfStmt *) child_block->stmt);} break;
            case AST_WHILELOOP: {TypecheckWhileLoop((WhileLoop *) child_block->stmt);} break;
            case AST_FORLOOP:   {TypecheckForLoop((ForLoop *) child_block->stmt);} break;
        }

        child_block = child_block->glue;
    }
}

static void TypecheckFuncDecl(FuncDecl *funcdecl) {
    TypecheckBlock(funcdecl->block);
}

void TypecheckFile(File *file) {
    File *child_file = file;
    while (child_file != 0 && child_file->funcdecl != 0) {
        func_idents[num_funcs] = strdup(child_file->funcdecl->ident);
        num_funcs += 1;
        child_file = child_file->glue;
    }

    child_file = file;
    while (child_file != 0 && child_file->funcdecl != 0) {
        TypecheckFuncDecl(child_file->funcdecl);
        child_file = child_file->glue;
    }
}

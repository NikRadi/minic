#include "Typechecking.h"


static void TypecheckCompoundStmt(struct AstNode *compoundstmt);


static void Assert(enum Bool expression) {
    if (!expression) {
        printf("Assertion failed\n");
        exit(1);
    }
}

static void TypecheckExpr(struct AstNode *expr) {
}

static void TypecheckVarDecl(struct AstNode *vardecl) {
    Assert(vardecl->type == AST_DECL);
}

static void TypecheckVarAssign(struct AstNode *varassign) {
    Assert(varassign->type == AST_ASSIGN);
    TypecheckExpr(varassign->rhs);
}

static void TypecheckIfStmt(struct AstNode *ifstmt) {
    Assert(ifstmt->type == AST_IF);
    TypecheckExpr(ifstmt->lhs);
    TypecheckCompoundStmt(ifstmt->rhs);
}

static void TypecheckWhileLoop(struct AstNode *whileloop) {
    Assert(whileloop->type == AST_WHILE);
    TypecheckExpr(whileloop->lhs);
    TypecheckCompoundStmt(whileloop->rhs);
}

static void TypecheckCompoundStmt(struct AstNode *compoundstmt) {
    struct AstNode *stmt = compoundstmt;
    while (stmt != 0 && stmt->lhs != 0) {
        Assert(stmt->type == AST_COMPOUND);
        switch (stmt->lhs->type) {
            case AST_PRINT:   {} break;
            case AST_DECL:    {TypecheckVarDecl(stmt->lhs);} break;
            case AST_ASSIGN : {TypecheckVarAssign(stmt->lhs);} break;
            case AST_IF:      {TypecheckIfStmt(stmt->lhs);} break;
            case AST_WHILE:   {TypecheckWhileLoop(stmt->lhs);} break;
            default: {
                printf("invalid statement in compound '%d'\n", stmt->lhs->type);
                exit(1);
            } break;
        }

        stmt = stmt->rhs;
    }
}

static void TypecheckFuncdecl(struct AstNode *funcdecl) {
    Assert(funcdecl->type == AST_FUNCTION);
    TypecheckCompoundStmt(funcdecl->rhs);
}

void TypecheckFile(struct AstNode *ast) {
    struct AstNode *stmt = ast;
    while (stmt != 0 && stmt->lhs != 0) {
        switch (stmt->lhs->type) {
            case AST_FUNCTION: {TypecheckFuncdecl(stmt->lhs);} break;
            default: {
                printf("invalid global statment\n");
                exit(1);
            } break;
        }

        stmt = stmt->rhs;
    }
}
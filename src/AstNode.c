#include "AstNode.h"
#include "ReportError.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW_TYPE(type) ((struct type *) malloc(sizeof(struct type)))

static struct Expr *NewExpr(enum ExprType type) {
    struct Expr *expr = NEW_TYPE(Expr);
    expr->lhs = NULL;
    expr->rhs = NULL;
    expr->int_value = 0;
    expr->rbp_offset = 0;
    expr->type = type;
    return expr;
}


//
// ===
// == Functions defined in AstNode.h
// ===
//


bool AreVariablesEquals(void *a, void *b) {
    struct Expr *e1 = (struct Expr *) a;
    struct Expr *e2 = (struct Expr *) b;
    return strcmp(e1->str_value, e2->str_value) == 0;
}

struct Expr *NewOperationAddExpr(struct Expr *lhs, struct Expr *rhs) {
    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_INTEGER) {
        return NewOperationExpr(EXPR_ADD, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_POINTER && rhs->operand_type == OPERAND_INTEGER) {
        struct Expr *num = NewNumberExpr(8);
        rhs = NewOperationExpr(EXPR_MUL, num, rhs);
        rhs->base_operand_type = rhs->operand_type;
        rhs->operand_type = OPERAND_POINTER;
        struct Expr *expr = NewOperationExpr(EXPR_ADD, lhs, rhs);
        expr->base_operand_type = rhs->operand_type;
        expr->operand_type = OPERAND_POINTER;
        return expr;
    }

    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_POINTER) {
        struct Expr *num = NewNumberExpr(8);
        lhs = NewOperationExpr(EXPR_MUL, lhs, num);
        lhs->base_operand_type = rhs->operand_type;
        lhs->operand_type = OPERAND_POINTER;
        struct Expr *expr = NewOperationExpr(EXPR_ADD, lhs, rhs);
        expr->base_operand_type = rhs->operand_type;
        expr->operand_type = OPERAND_POINTER;
        return expr;
    }

    ReportInternalError("binary add operation");
    return NULL;
}

struct Expr *NewOperationAddrExpr(struct Expr *lhs) {
    struct Expr *expr = NewOperationExpr(EXPR_ADDR, lhs, NULL);
    expr->operand_type = OPERAND_POINTER;
    expr->base_operand_type = (lhs->operand_type == OPERAND_POINTER) ? OPERAND_POINTER : OPERAND_INTEGER;
    return expr;
}

struct Expr *NewOperationExpr(enum ExprType type, struct Expr *lhs, struct Expr *rhs) {
    struct Expr *expr = NewExpr(type);
    expr->lhs = lhs;
    expr->rhs = rhs;
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct Expr *NewOperationSubExpr(struct Expr *lhs, struct Expr *rhs) {
    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_INTEGER) {
        return NewOperationExpr(EXPR_SUB, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_POINTER && rhs->operand_type == OPERAND_INTEGER) {
        struct Expr *num = NewNumberExpr(8);
        rhs = NewOperationExpr(EXPR_MUL, num, rhs); // TODO: Better solution for these
        rhs->base_operand_type = rhs->operand_type;
        rhs->operand_type = OPERAND_POINTER;
        struct Expr *expr = NewOperationExpr(EXPR_SUB, lhs, rhs);
        expr->base_operand_type = rhs->operand_type;
        expr->operand_type = OPERAND_POINTER;
        return expr;
    }

    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_POINTER) {
        struct Expr *num = NewNumberExpr(8);
        lhs = NewOperationExpr(EXPR_MUL, lhs, num);
        lhs->base_operand_type = rhs->operand_type;
        lhs->operand_type = OPERAND_POINTER;
        struct Expr *expr = NewOperationExpr(EXPR_SUB, lhs, rhs);
        expr->base_operand_type = rhs->operand_type;
        expr->operand_type = OPERAND_POINTER;
        return expr;
    }

    if (lhs->operand_type == OPERAND_POINTER && rhs->operand_type == OPERAND_POINTER) {
        lhs = NewOperationExpr(EXPR_SUB, lhs, rhs);
        struct Expr *num = NewNumberExpr(8);
        struct Expr *expr = NewOperationExpr(EXPR_DIV, lhs, num);
        return expr;
    }

    ReportInternalError("binary sub operation");
    return NULL;
}

struct Expr *NewNumberExpr(int value) {
    struct Expr *expr = NewExpr(EXPR_NUM);
    expr->int_value = value;
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct Expr *NewVariableExpr(char *value) {
    struct Expr *expr = NewExpr(EXPR_VAR);
    strncpy(expr->str_value, value, TOKEN_MAX_IDENTIFIER_LENGTH);
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct CompoundStatement *NewCompoundStatement(struct List statements) {
    struct CompoundStatement *compound_statement = NEW_TYPE(CompoundStatement);
    compound_statement->node.type = AST_COMPOUND_STATEMENT;
    compound_statement->statements = statements;
    return compound_statement;
}

struct ExpressionStatement *NewExpressionStatement(struct Expr *expr) {
    struct ExpressionStatement *expression_statement = NEW_TYPE(ExpressionStatement);
    expression_statement->node.type = AST_EXPRESSION_STATEMENT;
    expression_statement->expr = expr;
    return expression_statement;
}

struct ForStatement *NewForStatement(struct Expr *init_expr, struct Expr *cond_expr, struct Expr *loop_expr, struct AstNode *statement) {
    struct ForStatement *for_statement = NEW_TYPE(ForStatement);
    for_statement->node.type = AST_FOR_STATEMENT;
    for_statement->init_expr = init_expr;
    for_statement->cond_expr = cond_expr;
    for_statement->loop_expr = loop_expr;
    for_statement->statement = statement;
    return for_statement;
}

struct IfStatement *NewIfStatement(struct Expr *condition, struct AstNode *statement, struct AstNode *else_branch) {
    struct IfStatement *if_statement = NEW_TYPE(IfStatement);
    if_statement->node.type = AST_IF_STATEMENT;
    if_statement->condition = condition;
    if_statement->statement = statement;
    if_statement->else_branch = else_branch;
    return if_statement;
}

struct AstNode *NewNullStatement() {
    struct AstNode *null_statement = NEW_TYPE(AstNode);
    null_statement->type = AST_NULL_STATEMENT;
    return null_statement;
}

struct ReturnStatement *NewReturnStatement(struct Expr *expr) {
    struct ReturnStatement *return_statement = NEW_TYPE(ReturnStatement);
    return_statement->node.type = AST_RETURN_STATEMENT;
    return_statement->expr = expr;
    return return_statement;
}

struct WhileStatement *NewWhileStatement(struct Expr *condition, struct AstNode *statement) {
    struct WhileStatement *while_statement = NEW_TYPE(WhileStatement);
    while_statement->node.type = AST_WHILE_STATEMENT;
    while_statement->condition = condition;
    while_statement->statement = statement;
    return while_statement;
}

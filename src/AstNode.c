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

static struct Expr *NewPointerTo(enum ExprType base_type, enum ExprType type, struct Expr *lhs, struct Expr *rhs) {
    struct Expr *expr = NewOperationExpr(type, lhs, rhs);
    expr->base_operand_type = base_type;
    expr->operand_type = OPERAND_POINTER;
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

struct Expr *NewFunctionCallExpr(char *identifier, struct List args) {
    struct Expr *expr = NewExpr(EXPR_FUNC_CALL);
    strncpy(expr->str_value, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    expr->args = args;
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct Expr *NewOperationAddExpr(struct Expr *lhs, struct Expr *rhs) {
    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_INTEGER) {
        return NewOperationExpr(EXPR_ADD, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_POINTER && rhs->operand_type == OPERAND_INTEGER) {
        struct Expr *num = NewNumberExpr(8);
        rhs = NewPointerTo(rhs->operand_type, EXPR_MUL, num, rhs);
        return NewPointerTo(rhs->operand_type, EXPR_ADD, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_POINTER) {
        struct Expr *num = NewNumberExpr(8);
        lhs = NewPointerTo(rhs->operand_type, EXPR_MUL, lhs, num);
        return NewPointerTo(rhs->operand_type, EXPR_ADD, lhs, rhs);
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
        rhs = NewPointerTo(rhs->operand_type, EXPR_MUL, num, rhs);
        return NewPointerTo(rhs->operand_type, EXPR_SUB, lhs, rhs);
    }

    if (lhs->operand_type == OPERAND_INTEGER && rhs->operand_type == OPERAND_POINTER) {
        struct Expr *num = NewNumberExpr(8);
        lhs = NewPointerTo(rhs->operand_type, EXPR_MUL, lhs, num);
        return NewPointerTo(rhs->operand_type, EXPR_SUB, lhs, rhs);
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

struct Expr *NewVariableExpr(char *identifier) {
    struct Expr *expr = NewExpr(EXPR_VAR);
    strncpy(expr->str_value, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    expr->operand_type = OPERAND_INTEGER;
    return expr;
}

struct FunctionDefinition *NewFunctionDefinition(char *identifier) {
    struct FunctionDefinition *function = NEW_TYPE(FunctionDefinition);
    function->node.type = AST_FUNCTION_DEFINITION;

    function->num_params = 0;
    function->stack_size = 0;
    strncpy(function->identifier, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    function->body = NewCompoundStatement();
    List_Init(&function->var_declarations);

    return function;
}

struct TranslationUnit *NewTranslationUnit() {
    struct TranslationUnit *translation_unit = NEW_TYPE(TranslationUnit);
    translation_unit->node.type = AST_TRANSLATION_UNIT;
    List_Init(&translation_unit->functions);
    return translation_unit;
}

struct CompoundStatement *NewCompoundStatement() {
    struct CompoundStatement *compound_statement = NEW_TYPE(CompoundStatement);
    compound_statement->node.type = AST_COMPOUND_STATEMENT;
    List_Init(&compound_statement->statements);
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

static int indent = 0;
void PrintE(struct Expr *expr) {
    switch (expr->type) {
        case EXPR_NUM: {
            fprintf(stdout, "%*s<Num int_value=\"%d\" %d %d />\n", indent, "", 
                expr->int_value,
                expr->operand_type,
                expr->base_operand_type
            );
        } break;
        case EXPR_VAR: {
            fprintf(stdout, "%*s<Var str_value=\"%s\" %d %d />\n",
                indent, "",
                expr->str_value,
                expr->operand_type,
                expr->base_operand_type
            );
        } break;
        case EXPR_DEREF: {
            fprintf(stdout, "%*s<Deref>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Deref>\n", indent, "");
        } break;
        case EXPR_ADDR: {
            fprintf(stdout, "%*s<Addr %d %d>\n", indent, "",
                expr->operand_type,
                expr->base_operand_type
            );

            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Addr>\n", indent, "");
        } break;
        case EXPR_ADD: {
            fprintf(stdout, "%*s<Add>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Add>\n", indent, "");
        } break;
        case EXPR_SUB: {
            fprintf(stdout, "%*s<Sub>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Sub>\n", indent, "");
        } break;
        case EXPR_ASSIGN: {
            fprintf(stdout, "%*s<Assign>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Assign>\n", indent, "");
        } break;
        default: {
            fprintf(stdout, "%*s<UNKNOWN expr %d/>\n", indent, "", expr->type);
        } break;
    }
}

void PrintS(struct AstNode *node) {
    switch (node->type) {
        case AST_DECLARATION: {
            struct Declaration *d = (struct Declaration *) node;
            fprintf(stdout, "%*s<Declaration identifier=\"%s\" rbp_offset=\"%d\"/>\n",
                indent, "",
                d->identifier,
                d->rbp_offset
            );
        } break;
        case AST_FUNCTION_DEFINITION: {
            struct FunctionDefinition *f = (struct FunctionDefinition *) node;
            fprintf(stdout, "%*s<FunctionDefinition stack_size=\"%d\">\n", indent, "", f->stack_size);
            indent += 2;
            for (int i = 0; i < f->var_declarations.count; ++i) {
                struct AstNode *d = (struct AstNode *) List_Get(&f->var_declarations, i);
                PrintS(d);
            }

            PrintS((struct AstNode *) f->body);
            indent -= 2;
            fprintf(stdout, "%*s</FunctionDefinition>\n", indent, "");
        } break;
        case AST_COMPOUND_STATEMENT: {
            struct CompoundStatement *c = (struct CompoundStatement *) node;
            fprintf(stdout, "%*s<CompoundStatement>\n", indent, "");
            indent += 2;
            for (int i = 0; i < c->statements.count; ++i) {
                struct AstNode *s = (struct AstNode *) List_Get(&c->statements, i);
                PrintS(s);
            }

            indent -= 2;
            fprintf(stdout, "%*s</CompoundStatement>\n", indent, "");
        } break;
        case AST_EXPRESSION_STATEMENT: {
            struct ExpressionStatement *e = (struct ExpressionStatement *) node;
            fprintf(stdout, "%*s<ExpressionStatement>\n", indent, "");
            indent += 2;
            PrintE(e->expr);
            indent -= 2;
            fprintf(stdout, "%*s</ExpressionStatement>\n", indent, "");
        } break;
        case AST_RETURN_STATEMENT: {
            struct ReturnStatement *r = (struct ReturnStatement *) node;
            fprintf(stdout, "%*s<ReturnStatement>\n", indent, "");
            indent += 2;
            PrintE(r->expr);
            indent -= 2;
            fprintf(stdout, "%*s</ReturnStatement>\n", indent, "");
        } break;
        default: {
            fprintf(stdout, "%*s<UNKNOWN statement %d/>\n", indent, "", node->type);
        } break;
    }
}

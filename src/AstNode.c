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
    expr->id = -1;
    expr->rbp_offset = 0;
    expr->type = type;
    expr->operand_type = PRIMTYPE_INVALID;
    expr->base_operand_type = PRIMTYPE_INVALID;
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
    return expr;
}

struct Expr *NewOperationExpr(enum ExprType type, struct Expr *lhs, struct Expr *rhs) {
    struct Expr *expr = NewExpr(type);
    expr->lhs = lhs;
    expr->rhs = rhs;
    return expr;
}

struct Expr *NewNumberExpr(int value) {
    struct Expr *expr = NewExpr(EXPR_NUM);
    expr->int_value = value;
    return expr;
}

struct Expr *NewStringExpr(char *value) {
    struct Expr *expr = NewExpr(EXPR_STR);
    strncpy(expr->str_value, value, TOKEN_MAX_IDENTIFIER_LENGTH);
    return expr;
}

struct Expr *NewVariableExpr(char *identifier) {
    struct Expr *expr = NewExpr(EXPR_VAR);
    strncpy(expr->str_value, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    return expr;
}

struct Declarator *NewDeclarator() {
    struct Declarator *declarator = NEW_TYPE(Declarator);
    declarator->node.type = AST_DECLARATOR;
    declarator->value = NULL;
    declarator->array_dimensions = 0;
    declarator->pointer_inderection = 0;
    return declarator;
}

struct VarDeclaration *NewVarDeclaration() {
    struct VarDeclaration *var_declaration = NEW_TYPE(VarDeclaration);
    var_declaration->node.type = AST_VAR_DECLARATION;
    List_Init(&var_declaration->declarators);
    var_declaration->type = PRIMTYPE_INVALID;
    return var_declaration;
}

struct FunctionDef *NewFunctionDef(char *identifier) {
    struct FunctionDef *function = NEW_TYPE(FunctionDef);
    function->node.type = AST_FUNCTION_DEF;

    function->num_params = 0;
    function->stack_size = 0;
    strncpy(function->identifier, identifier, TOKEN_MAX_IDENTIFIER_LENGTH);
    function->body = NewCompoundStmt();
    List_Init(&function->var_decls);
    List_Init(&function->params);

    return function;
}

struct TranslationUnit *NewTranslationUnit() {
    struct TranslationUnit *translation_unit = NEW_TYPE(TranslationUnit);
    translation_unit->node.type = AST_TRANSLATION_UNIT;
    List_Init(&translation_unit->functions);
    List_Init(&translation_unit->data_fields);
    return translation_unit;
}

struct CompoundStmt *NewCompoundStmt() {
    struct CompoundStmt *compound_stmt = NEW_TYPE(CompoundStmt);
    compound_stmt->node.type = AST_COMPOUND_STMT;
    List_Init(&compound_stmt->body);
    return compound_stmt;
}

struct ExpressionStmt *NewExpressionStmt(struct Expr *expr) {
    struct ExpressionStmt *expression_stmt = NEW_TYPE(ExpressionStmt);
    expression_stmt->node.type = AST_EXPRESSION_STMT;
    expression_stmt->expr = expr;
    return expression_stmt;
}

struct ForStmt *NewForStmt(struct Expr *init_expr, struct Expr *cond_expr, struct Expr *loop_expr, struct AstNode *stmt) {
    struct ForStmt *for_stmt = NEW_TYPE(ForStmt);
    for_stmt->node.type = AST_FOR_STMT;
    for_stmt->init_expr = init_expr;
    for_stmt->cond_expr = cond_expr;
    for_stmt->loop_expr = loop_expr;
    for_stmt->stmt = stmt;
    return for_stmt;
}

struct IfStmt *NewIfStmt(struct Expr *condition, struct AstNode *stmt, struct AstNode *else_branch) {
    struct IfStmt *if_stmt = NEW_TYPE(IfStmt);
    if_stmt->node.type = AST_IF_STMT;
    if_stmt->condition = condition;
    if_stmt->stmt = stmt;
    if_stmt->else_branch = else_branch;
    return if_stmt;
}

struct AstNode *NewNullStmt() {
    struct AstNode *null_stmt = NEW_TYPE(AstNode);
    null_stmt->type = AST_NULL_STMT;
    return null_stmt;
}

struct ReturnStmt *NewReturnStmt(struct Expr *expr) {
    struct ReturnStmt *return_stmt = NEW_TYPE(ReturnStmt);
    return_stmt->node.type = AST_RETURN_STMT;
    return_stmt->expr = expr;
    return return_stmt;
}

struct WhileStmt *NewWhileStmt(struct Expr *condition, struct AstNode *stmt) {
    struct WhileStmt *while_stmt = NEW_TYPE(WhileStmt);
    while_stmt->node.type = AST_WHILE_STMT;
    while_stmt->condition = condition;
    while_stmt->stmt = stmt;
    return while_stmt;
}

static int indent = 0;
void PrintE(struct Expr *expr) {
    switch (expr->type) {
        case EXPR_NUM: {
            fprintf(stdout, "%*s<Num int_value=\"%d\" %d />\n", indent, "", expr->int_value, expr->operand_type);
        } break;
        case EXPR_STR: {
            fprintf(stdout, "%*s<Str str_value=\"%s\" />\n", indent, "", expr->str_value);
        } break;
        case EXPR_VAR: {
            fprintf(stdout, "%*s<Var str_value=\"%s\" %d />\n", indent, "", expr->str_value, expr->operand_type);
        } break;
        case EXPR_FUNC_CALL: {
            fprintf(stdout, "%*s<FuncCall identifier=\"%s\">\n", indent, "", expr->str_value);
            indent += 2;
            struct List *args = &expr->args;
            for (int i = 0; i < args->count; ++i) {
                struct Expr *arg = (struct Expr *) List_Get(args, i);
                PrintE(arg);
            }

            indent -= 2;
            fprintf(stdout, "%*s</FuncCall>\n", indent, "");
        } break;
        case EXPR_PLUS: {
            fprintf(stdout, "%*s<UnaryPlus>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</UnaryPlus>\n", indent, "");
        } break;
        case EXPR_NEG: {
            fprintf(stdout, "%*s<Neg>\n", indent, "");
            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Neg>\n", indent, "");
        } break;
        case EXPR_DEREF: {
            fprintf(stdout, "%*s<Deref %d>\n", indent, "", expr->operand_type);
            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Deref>\n", indent, "");
        } break;
        case EXPR_ADDR: {
            fprintf(stdout, "%*s<Addr %d>\n", indent, "", expr->operand_type);
            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Addr>\n", indent, "");
        } break;
        case EXPR_SIZEOF: {
            fprintf(stdout, "%*s<Sizeof %d>\n", indent, "", expr->operand_type);
            indent += 2;
            PrintE(expr->lhs);
            indent -= 2;
            fprintf(stdout, "%*s</Sizeof>\n", indent, "");
        } break;
        case EXPR_ADD: {
            fprintf(stdout, "%*s<Add %d>\n", indent, "", expr->operand_type);
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
        case EXPR_MUL: {
            fprintf(stdout, "%*s<Mul %d>\n", indent, "", expr->operand_type);
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Mul>\n", indent, "");
        } break;
        case EXPR_DIV: {
            fprintf(stdout, "%*s<Div %d>\n", indent, "", expr->operand_type);
            indent += 2;
            PrintE(expr->lhs);
            PrintE(expr->rhs);
            indent -= 2;
            fprintf(stdout, "%*s</Div>\n", indent, "");
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
        case AST_DECLARATOR: {
            struct Declarator *d = (struct Declarator *) node;
            if (d->value) {
                fprintf(stdout, "%*s<Declarator identifier=\"%s\" array_dimensions=\"%d\">\n",
                    indent,
                    "",
                    d->identifier,
                    d->array_dimensions
                );

                indent += 2;
                PrintE(d->value);
                indent -= 2;
            }
            else {
                fprintf(stdout, "%*s</Declarator identifier=\"%s\" array_dimensions=\"%d\">\n",
                    indent,
                    "",
                    d->identifier,
                    d->array_dimensions
                );
            }
        } break;
        case AST_VAR_DECLARATION: {
            struct VarDeclaration *d = (struct VarDeclaration *) node;
            fprintf(stdout, "%*s<VarDeclaration\">\n", indent, "");
            indent += 2;
            for (int i = 0; i < d->declarators.count; ++i) {
                struct AstNode *declaration = (struct AstNode *) List_Get(&d->declarators, i);
                PrintS(declaration);
            }

            indent -= 2;
            fprintf(stdout, "%*s</VarDeclaration>\n", indent, "");
        } break;
        case AST_FUNCTION_DEF: {
            struct FunctionDef *f = (struct FunctionDef *) node;
            fprintf(stdout, "%*s<FunctionDef stack_size=\"%d\">\n", indent, "", f->stack_size);
            indent += 2;
            for (int i = 0; i < f->var_decls.count; ++i) {
                struct AstNode *d = (struct AstNode *) List_Get(&f->var_decls, i);
                PrintS(d);
            }

            PrintS((struct AstNode *) f->body);
            indent -= 2;
            fprintf(stdout, "%*s</FunctionDef>\n", indent, "");
        } break;
        case AST_COMPOUND_STMT: {
            struct CompoundStmt *c = (struct CompoundStmt *) node;
            fprintf(stdout, "%*s<CompoundStmt>\n", indent, "");
            indent += 2;
            for (int i = 0; i < c->body.count; ++i) {
                struct AstNode *s = (struct AstNode *) List_Get(&c->body, i);
                PrintS(s);
            }

            indent -= 2;
            fprintf(stdout, "%*s</CompoundStmt>\n", indent, "");
        } break;
        case AST_EXPRESSION_STMT: {
            struct ExpressionStmt *e = (struct ExpressionStmt *) node;
            fprintf(stdout, "%*s<ExpressionStmt>\n", indent, "");
            indent += 2;
            PrintE(e->expr);
            indent -= 2;
            fprintf(stdout, "%*s</ExpressionStmt>\n", indent, "");
        } break;
        case AST_RETURN_STMT: {
            struct ReturnStmt *r = (struct ReturnStmt *) node;
            fprintf(stdout, "%*s<ReturnStmt>\n", indent, "");
            indent += 2;
            PrintE(r->expr);
            indent -= 2;
            fprintf(stdout, "%*s</ReturnStmt>\n", indent, "");
        } break;
        case AST_TRANSLATION_UNIT: {
            struct TranslationUnit *t = (struct TranslationUnit *) node;
//            fprintf(stdout, "%*s<TranslationUnit>\n", indent, "");
            indent += 2;
            for (int i = 0; i < t->functions.count; ++i) {
                struct AstNode *f = (struct AstNode *) List_Get(&t->functions, i);
                PrintS(f);
            }

            indent -= 2;
            fprintf(stdout, "%*s</TranslationUnit>\n", indent, "");
        } break;
        default: {
            fprintf(stdout, "%*s<UNKNOWN statement %d/>\n", indent, "", node->type);
        } break;
    }
}

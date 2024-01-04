#include "Parser.h"
#include "AstNode.h"
#include "ReportError.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW_TYPE(type) ((struct type *) malloc(sizeof(struct type)))

static struct CompoundStatement *ParseCompoundStatement();
static struct AstNode *ParseStatement();

static struct FunctionDefinition *current_function;
static struct Lexer *l;

static void ExpectAndEat(enum TokenType type) {
    struct Token token = Lexer_PeekToken(l);
    if (token.type != type) {
        ReportErrorAtToken(l, token, "expected %d", type);
    }

    Lexer_EatToken(l);
}


//
// ===
// == Parse expressions
// ===
//


struct OperatorParseData;
typedef struct Expr *(*ParseOperatorFunction)(struct OperatorParseData);

static struct Expr *ParseBinaryAddOp(struct OperatorParseData data);
static struct Expr *ParseBinaryOp(struct OperatorParseData data);
static struct Expr *ParseBinarySubOp(struct OperatorParseData data);
static struct Expr *ParseBracket(struct OperatorParseData data);
static struct Expr *ParseExpr(int precedence);
static struct Expr *ParseNumber(struct OperatorParseData data);
static struct Expr *ParseUnaryAddrOp(struct OperatorParseData data);
static struct Expr *ParseUnaryOp(struct OperatorParseData data);
static struct Expr *ParseUnaryPlusOp(struct OperatorParseData data);
static struct Expr *ParseVariable(struct OperatorParseData data);

struct OperatorParseData {
    int precedence;
    bool is_right_associative;
    enum ExprType type;
    struct Expr *lhs;
    ParseOperatorFunction Parse;
};

static struct OperatorParseData infix_operators[TOKEN_COUNT] = {
    [TOKEN_EQUALS]                  = { .precedence = 10, .type = EXPR_ASSIGN,  .Parse = ParseBinaryOp, .is_right_associative = true },
    [TOKEN_2_EQUALS]                = { .precedence = 20, .type = EXPR_EQU,     .Parse = ParseBinaryOp },
    [TOKEN_EXCLAMATION_MARK_EQUALS] = { .precedence = 20, .type = EXPR_NEQ,     .Parse = ParseBinaryOp },
    [TOKEN_LESS_THAN]               = { .precedence = 30, .type = EXPR_LT,      .Parse = ParseBinaryOp },
    [TOKEN_LESS_THAN_EQUALS]        = { .precedence = 30, .type = EXPR_LTE,     .Parse = ParseBinaryOp },
    [TOKEN_GREATER_THAN]            = { .precedence = 30, .type = EXPR_GT,      .Parse = ParseBinaryOp },
    [TOKEN_GREATER_THAN_EQUALS]     = { .precedence = 30, .type = EXPR_GTE,     .Parse = ParseBinaryOp },
    [TOKEN_PLUS]                    = { .precedence = 40, .type = EXPR_ADD,     .Parse = ParseBinaryAddOp },
    [TOKEN_MINUS]                   = { .precedence = 40, .type = EXPR_SUB,     .Parse = ParseBinarySubOp },
    [TOKEN_STAR]                    = { .precedence = 50, .type = EXPR_MUL,     .Parse = ParseBinaryOp },
    [TOKEN_SLASH]                   = { .precedence = 50, .type = EXPR_DIV,     .Parse = ParseBinaryOp },
};

static struct OperatorParseData prefix_operators[TOKEN_COUNT] = {
    [TOKEN_PLUS]                    = { .precedence = 60, .type = EXPR_NEG,     .Parse = ParseUnaryPlusOp },
    [TOKEN_MINUS]                   = { .precedence = 60, .type = EXPR_NEG,     .Parse = ParseUnaryOp },
    [TOKEN_STAR]                    = { .precedence = 60, .type = EXPR_DEREF,   .Parse = ParseUnaryOp },
    [TOKEN_AMPERSAND]               = { .precedence = 60, .type = EXPR_ADDR,    .Parse = ParseUnaryAddrOp },
    [TOKEN_IDENTIFIER]              = { .Parse = ParseVariable },
    [TOKEN_LEFT_ROUND_BRACKET]      = { .Parse = ParseBracket },
    [TOKEN_LITERAL_NUMBER]          = { .Parse = ParseNumber },
};

static struct Expr *ParseBinaryAddOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *rhs = ParseExpr(data.precedence - data.is_right_associative);
    return NewOperationAddExpr(data.lhs, rhs);
}

static struct Expr *ParseBinaryOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *rhs = ParseExpr(data.precedence - data.is_right_associative);
    return NewOperationExpr(data.type, data.lhs, rhs);
}

static struct Expr *ParseBinarySubOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *rhs = ParseExpr(data.precedence - data.is_right_associative);
    return NewOperationSubExpr(data.lhs, rhs);
}

static struct Expr *ParseBracket(struct OperatorParseData data) {
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    struct Expr *expr = ParseExpr(0);
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
    return expr;
}

static struct Expr *ParseExpr(int precedence) {
    struct Token token = Lexer_PeekToken(l);
    struct OperatorParseData prefix_op = prefix_operators[token.type];
    if (!prefix_op.Parse) {
        ReportErrorAtToken(l, token, "expected expression");
    }

    struct Expr *lhs = prefix_op.Parse(prefix_op);
    token = Lexer_PeekToken(l);
    struct OperatorParseData infix_op = infix_operators[token.type];
    while (precedence < infix_op.precedence) {
        infix_op.lhs = lhs;
        lhs = infix_op.Parse(infix_op);
        token = Lexer_PeekToken(l);
        infix_op = infix_operators[token.type];
    }

    return lhs;
}

static struct Expr *ParseNumber(struct OperatorParseData data) {
    int value = Lexer_PeekToken(l).int_value;
    Lexer_EatToken(l);
    return NewNumberExpr(value);
}

static struct Expr *ParseUnaryAddrOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *lhs = ParseExpr(data.precedence);
    return NewOperationAddrExpr(lhs);
}

static struct Expr *ParseUnaryOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *lhs = ParseExpr(data.precedence);
    return NewOperationExpr(data.type, lhs, NULL);
}

static struct Expr *ParseUnaryPlusOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    return ParseExpr(data.precedence);
}

static struct Expr *ParseVariable(struct OperatorParseData data) {
    char *value = Lexer_PeekToken(l).str_value;
    Lexer_EatToken(l);

    struct Expr *var = NewVariableExpr(value);
    struct List *function_vars = &current_function->variables;
    struct Expr *function_var = (struct Expr *) List_Find(function_vars, var, AreVariablesEquals);
    if (!function_var) {
        List_Add(function_vars, (void *) var);
    }

    return var;
}


//
// ===
// == Parse statements
// ===
//


static struct CompoundStatement *ParseCompoundStatement() {
    ExpectAndEat(TOKEN_LEFT_CURLY_BRACKET);
    struct List statements;
    List_Init(&statements);
    while (Lexer_PeekToken(l).type != TOKEN_RIGHT_CURLY_BRACKET) {
        struct AstNode *statement = ParseStatement();
        List_Add(&statements, statement);
    }

    Lexer_EatToken(l); // TOKEN_RIGHT_CURLY_BRACKET
    return NewCompoundStatement(statements);
}

static struct ExpressionStatement *ParseExpressionStatement() {
    struct Expr *expr = ParseExpr(0);
    ExpectAndEat(TOKEN_SEMICOLON);
    return NewExpressionStatement(expr);
}

static struct ForStatement *ParseForStatement() {
    ExpectAndEat(TOKEN_KEYWORD_FOR);
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    struct Expr *init_expr = NULL;
    if (Lexer_PeekToken(l).type != TOKEN_SEMICOLON) {
        init_expr = ParseExpr(0);
    }

    ExpectAndEat(TOKEN_SEMICOLON);
    struct Expr *cond_expr = NULL;
    if (Lexer_PeekToken(l).type != TOKEN_SEMICOLON) {
        cond_expr = ParseExpr(0);
    }

    ExpectAndEat(TOKEN_SEMICOLON);
    struct Expr *loop_expr = NULL;
    if (Lexer_PeekToken(l).type != TOKEN_RIGHT_ROUND_BRACKET) {
        loop_expr = ParseExpr(0);
    }

    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
    struct AstNode *statement = ParseStatement();
    return NewForStatement(init_expr, cond_expr, loop_expr, statement);
}

static struct AstNode *ParseNullStatement() {
    ExpectAndEat(TOKEN_SEMICOLON);
    return NewNullStatement();
}

static struct IfStatement *ParseIfStatement() {
    ExpectAndEat(TOKEN_KEYWORD_IF);
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    struct Expr *condition = ParseExpr(0);
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);

    struct AstNode *statement = ParseStatement();
    struct AstNode *else_branch = NULL;
    if (Lexer_PeekToken(l).type == TOKEN_KEYWORD_ELSE) {
        Lexer_EatToken(l);
        else_branch = ParseStatement();
    }

    return NewIfStatement(condition, statement, else_branch);
}

static struct ReturnStatement *ParseReturnStatement() {
    ExpectAndEat(TOKEN_KEYWORD_RETURN);
    struct Expr *expr = NULL;
    if (Lexer_PeekToken(l).type != TOKEN_SEMICOLON) {
        expr = ParseExpr(0);
    }

    ExpectAndEat(TOKEN_SEMICOLON);
    return NewReturnStatement(expr);
}

static struct WhileStatement *ParseWhileStatement() {
    ExpectAndEat(TOKEN_KEYWORD_WHILE);
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    struct Expr *condition = ParseExpr(0);
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
    struct AstNode *statement = ParseStatement();
    return NewWhileStatement(condition, statement);
}

static struct AstNode *ParseStatement() {
    struct Token token = Lexer_PeekToken(l);
    switch (token.type) {
        case TOKEN_SEMICOLON:           return                    ParseNullStatement();
        case TOKEN_LEFT_CURLY_BRACKET:  return (struct AstNode *) ParseCompoundStatement();
        case TOKEN_KEYWORD_FOR:         return (struct AstNode *) ParseForStatement();
        case TOKEN_KEYWORD_IF:          return (struct AstNode *) ParseIfStatement();
        case TOKEN_KEYWORD_RETURN:      return (struct AstNode *) ParseReturnStatement();
        case TOKEN_KEYWORD_WHILE:       return (struct AstNode *) ParseWhileStatement();
        default:                        return (struct AstNode *) ParseExpressionStatement();
    }
}


//
// ===
// == Functions defined in Parser.h
// ===
//


struct FunctionDefinition *Parser_MakeAst(struct Lexer *lexer) {
    l = lexer;
    Lexer_EatToken(l);

    struct FunctionDefinition *function = NEW_TYPE(FunctionDefinition);
    function->node.type = AST_FUNCTION_DEFINITION,
    function->stack_size = 0;
    current_function = function;

    List_Init(&function->statements);
    List_Init(&function->variables);
    while (Lexer_PeekToken(l).type != TOKEN_END_OF_FILE) {
        struct AstNode *statement = ParseStatement();
        List_Add(&function->statements, statement);
    }

    return function;
}

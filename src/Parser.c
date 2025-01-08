#include "Parser.h"
#include "AstNode.h"
#include "ReportError.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW_TYPE(type) ((struct type *) malloc(sizeof(struct type)))

static struct CompoundStmt *ParseCompoundStmt();
static struct ExpressionStmt *ParseExpressionStmt();
static struct AstNode *ParseStmt();

static struct FunctionDefinition *current_function;
static struct Lexer *l;

static void ExpectAndEat(enum TokenType type) {
    struct Token token = Lexer_PeekToken(l);
    if (token.type != type) {
        ReportErrorAtToken(l, token, "expected %d but got %d", type, token.type);
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
static struct Expr *ParseIdentifier(struct OperatorParseData data);
static struct Expr *ParseNumber(struct OperatorParseData data);
static struct Expr *ParseUnaryAddrOp(struct OperatorParseData data);
static struct Expr *ParseUnaryOp(struct OperatorParseData data);
static struct Expr *ParseUnaryPlusOp(struct OperatorParseData data);

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
    [TOKEN_PLUS]                    = { .precedence = 60,                       .Parse = ParseUnaryPlusOp },
    [TOKEN_MINUS]                   = { .precedence = 60, .type = EXPR_NEG,     .Parse = ParseUnaryOp },
    [TOKEN_STAR]                    = { .precedence = 60, .type = EXPR_DEREF,   .Parse = ParseUnaryOp },
    [TOKEN_AMPERSAND]               = { .precedence = 60, .type = EXPR_ADDR,    .Parse = ParseUnaryAddrOp },
    [TOKEN_IDENTIFIER]              = {                                         .Parse = ParseIdentifier },
    [TOKEN_LEFT_ROUND_BRACKET]      = {                                         .Parse = ParseBracket },
    [TOKEN_LITERAL_NUMBER]          = {                                         .Parse = ParseNumber },
};


static struct Expr *ParseBinaryAddOp(struct OperatorParseData data) {
    ExpectAndEat(TOKEN_PLUS);
    struct Expr *rhs = ParseExpr(data.precedence - data.is_right_associative);
    return NewOperationAddExpr(data.lhs, rhs);
}

static struct Expr *ParseBinaryOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *rhs = ParseExpr(data.precedence - data.is_right_associative);
    return NewOperationExpr(data.type, data.lhs, rhs);
}

static struct Expr *ParseBinarySubOp(struct OperatorParseData data) {
    ExpectAndEat(TOKEN_MINUS);
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

static struct Expr *ParseIdentifier(struct OperatorParseData data) {
    char *identifier = Lexer_PeekToken(l).str_value;
    ExpectAndEat(TOKEN_IDENTIFIER);

    // Function call
    if (Lexer_PeekToken(l).type == TOKEN_LEFT_ROUND_BRACKET) {
        ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
        struct List args;
        List_Init(&args);
        while (Lexer_PeekToken(l).type != TOKEN_RIGHT_ROUND_BRACKET) {
            struct Expr *expr = ParseExpr(0);
            List_Add(&args, expr);
            if (Lexer_PeekToken(l).type == TOKEN_COMMA) {
                Lexer_EatToken(l);
            }
        }

        ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
        return NewFunctionCallExpr(identifier, args);
    }

    struct List *var_decls = &current_function->var_decls;
    struct Decl *var_decl = NULL;
    for (int i = 0; i < var_decls->count; ++i) {
        struct Decl *v = (struct Decl *) List_Get(var_decls, i);
        if (strcmp(v->identifier, identifier) == 0) {
            var_decl = v;
            break;
        }
    }

    struct Expr *variable = NewVariableExpr(identifier);
    if (var_decl->node.type == AST_DECL_ARRAY) {
        variable->operand_type = OPERAND_POINTER;
    }

    return variable;
}

static struct Expr *ParseNumber(struct OperatorParseData data) {
    int value = Lexer_PeekToken(l).int_value;
    ExpectAndEat(TOKEN_LITERAL_NUMBER);
    return NewNumberExpr(value);
}

static struct Expr *ParseUnaryAddrOp(struct OperatorParseData data) {
    ExpectAndEat(TOKEN_AMPERSAND);
    struct Expr *lhs = ParseExpr(data.precedence);
    return NewOperationAddrExpr(lhs);
}

static struct Expr *ParseUnaryOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *lhs = ParseExpr(data.precedence);
    return NewOperationExpr(data.type, lhs, NULL);
}

static struct Expr *ParseUnaryPlusOp(struct OperatorParseData data) {
    ExpectAndEat(TOKEN_PLUS);
    return ParseExpr(data.precedence);
}


//
// ===
// == Parse statements
// ===
//


static struct CompoundStmt *ParseCompoundStmt() {
    struct CompoundStmt *compound_stmt = NewCompoundStmt();
    struct List *stmts = &compound_stmt->stmts;
    struct List *var_decls = &current_function->var_decls;

    ExpectAndEat(TOKEN_LEFT_CURLY_BRACKET);
    while (Lexer_PeekToken(l).type != TOKEN_RIGHT_CURLY_BRACKET) {
        struct Token token = Lexer_PeekToken(l);
        if (token.type == TOKEN_KEYWORD_INT) {
            // Decl specifiers
            ExpectAndEat(TOKEN_KEYWORD_INT);

            do {
                // Declarator
                while (Lexer_PeekToken(l).type == TOKEN_STAR) {
                    Lexer_EatToken(l);
                }

                token = Lexer_PeekToken(l);
                struct Decl *d = (struct Decl *) malloc(sizeof(struct Decl));
                d->node.type = AST_DECL;
                strncpy(d->identifier, token.str_value, TOKEN_MAX_IDENTIFIER_LENGTH);
                List_Add(var_decls, d);

                if (Lexer_PeekToken2(l, 1).type == TOKEN_EQUALS) {
                    struct Expr *expr = ParseExpr(0);
                    struct ExpressionStmt *stmt = NewExpressionStmt(expr);
                    List_Add(stmts, stmt);
                }
                else {
                    Lexer_EatToken(l); // Eat the identifier
                }

                if (Lexer_PeekToken(l).type == TOKEN_SEMICOLON) {
                    break;
                }
                else if (Lexer_PeekToken(l).type == TOKEN_LEFT_SQUARE_BRACKET) {
                    ExpectAndEat(TOKEN_LEFT_SQUARE_BRACKET);

                    struct Token token = Lexer_PeekToken(l);
                    ExpectAndEat(TOKEN_LITERAL_NUMBER);

                    d->node.type = AST_DECL_ARRAY;
                    d->array_size = token.int_value;
                    ExpectAndEat(TOKEN_RIGHT_SQUARE_BRACKET);
                    break;
                }
                else {
                    ExpectAndEat(TOKEN_COMMA);
                }
            } while (true);
            ExpectAndEat(TOKEN_SEMICOLON);
        }
        else {
            struct AstNode *stmt = ParseStmt();
            List_Add(stmts, stmt);
        }
    }

    ExpectAndEat(TOKEN_RIGHT_CURLY_BRACKET);
    return compound_stmt;
}

static struct ExpressionStmt *ParseExpressionStmt() {
    struct Expr *expr = ParseExpr(0);
    ExpectAndEat(TOKEN_SEMICOLON);
    return NewExpressionStmt(expr);
}

static struct ForStmt *ParseForStmt() {
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
    struct AstNode *stmt = ParseStmt();
    return NewForStmt(init_expr, cond_expr, loop_expr, stmt);
}

static struct AstNode *ParseNullStmt() {
    ExpectAndEat(TOKEN_SEMICOLON);
    return NewNullStmt();
}

static struct IfStmt *ParseIfStmt() {
    ExpectAndEat(TOKEN_KEYWORD_IF);
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    struct Expr *condition = ParseExpr(0);
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);

    struct AstNode *stmt = ParseStmt();
    struct AstNode *else_branch = NULL;
    if (Lexer_PeekToken(l).type == TOKEN_KEYWORD_ELSE) {
        Lexer_EatToken(l);
        else_branch = ParseStmt();
    }

    return NewIfStmt(condition, stmt, else_branch);
}

static struct ReturnStmt *ParseReturnStmt() {
    ExpectAndEat(TOKEN_KEYWORD_RETURN);
    struct Expr *expr = NULL;
    if (Lexer_PeekToken(l).type != TOKEN_SEMICOLON) {
        expr = ParseExpr(0);
    }

    ExpectAndEat(TOKEN_SEMICOLON);
    return NewReturnStmt(expr);
}

static struct WhileStmt *ParseWhileStmt() {
    ExpectAndEat(TOKEN_KEYWORD_WHILE);
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    struct Expr *condition = ParseExpr(0);
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
    struct AstNode *stmt = ParseStmt();
    return NewWhileStmt(condition, stmt);
}

static struct AstNode *ParseStmt() {
    struct Token token = Lexer_PeekToken(l);
    switch (token.type) {
        case TOKEN_SEMICOLON:           return                    ParseNullStmt();
        case TOKEN_LEFT_CURLY_BRACKET:  return (struct AstNode *) ParseCompoundStmt();
        case TOKEN_KEYWORD_FOR:         return (struct AstNode *) ParseForStmt();
        case TOKEN_KEYWORD_IF:          return (struct AstNode *) ParseIfStmt();
        case TOKEN_KEYWORD_RETURN:      return (struct AstNode *) ParseReturnStmt();
        case TOKEN_KEYWORD_WHILE:       return (struct AstNode *) ParseWhileStmt();
        default:                        return (struct AstNode *) ParseExpressionStmt();
    }
}


//
// ===
// == Parse statements
// ===
//


struct FunctionDefinition *ParseFunctionDefinition() {
    // Decl specifiers
    ExpectAndEat(TOKEN_KEYWORD_INT);

    char *identifier = Lexer_PeekToken(l).str_value;

    struct FunctionDefinition *function = NewFunctionDefinition(identifier);
    current_function = function;

    ExpectAndEat(TOKEN_IDENTIFIER);
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    while (Lexer_PeekToken(l).type != TOKEN_RIGHT_ROUND_BRACKET) {
        // Decl specifiers
        ExpectAndEat(TOKEN_KEYWORD_INT);

        struct Decl *d = (struct Decl *) malloc(sizeof(struct Decl));
        d->node.type = AST_DECL;

        struct Token token = Lexer_PeekToken(l);
        strncpy(d->identifier, token.str_value, TOKEN_MAX_IDENTIFIER_LENGTH);
        List_Add(&function->var_decls, d);

        ExpectAndEat(TOKEN_IDENTIFIER);
        function->num_params += 1;
        if (Lexer_PeekToken(l).type == TOKEN_COMMA) {
            Lexer_EatToken(l);
        }
    }

    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
    function->body = ParseCompoundStmt();

    return function;
}

struct TranslationUnit *ParseTranslationUnit() {
    struct TranslationUnit *t_unit = NewTranslationUnit();
    while (Lexer_PeekToken(l).type != TOKEN_END_OF_FILE) {
        struct FunctionDefinition *function = ParseFunctionDefinition();
        List_Add(&t_unit->functions, function);
    }

    return t_unit;
}


//
// ===
// == Functions defined in Parser.h
// ===
//


struct TranslationUnit *Parser_MakeAst(struct Lexer *lexer) {
    l = lexer;
    return ParseTranslationUnit();
}

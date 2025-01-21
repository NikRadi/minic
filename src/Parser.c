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

static struct Lexer *l;

static void ExpectAndEat(enum TokenType type) {
    struct Token token = Lexer_PeekToken(l);
    if (token.type != type) {
        ReportErrorAtToken(l, token, "expected %d but got %d", type, token.type);
    }

    Lexer_EatToken(l);
}


struct OperatorParseData;
typedef struct Expr *(*ParseOperatorFunction)(struct OperatorParseData);

static struct Expr *ParseBinaryOp(struct OperatorParseData data);
static struct Expr *ParseBracket(struct OperatorParseData data);
static struct Expr *ParseExpr(int precedence);
static struct Expr *ParseIdentifier(struct OperatorParseData data);
static struct Expr *ParseNumber(struct OperatorParseData data);
static struct Expr *ParseString(struct OperatorParseData data);
static struct Expr *ParseUnaryOp(struct OperatorParseData data);

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
    [TOKEN_PLUS]                    = { .precedence = 40, .type = EXPR_ADD,     .Parse = ParseBinaryOp },
    [TOKEN_MINUS]                   = { .precedence = 40, .type = EXPR_SUB,     .Parse = ParseBinaryOp },
    [TOKEN_STAR]                    = { .precedence = 50, .type = EXPR_MUL,     .Parse = ParseBinaryOp },
    [TOKEN_SLASH]                   = { .precedence = 50, .type = EXPR_DIV,     .Parse = ParseBinaryOp },
};

static struct OperatorParseData prefix_operators[TOKEN_COUNT] = {
    [TOKEN_PLUS]                    = { .precedence = 60, .type = EXPR_PLUS,    .Parse = ParseUnaryOp },
    [TOKEN_MINUS]                   = { .precedence = 60, .type = EXPR_NEG,     .Parse = ParseUnaryOp },
    [TOKEN_STAR]                    = { .precedence = 60, .type = EXPR_DEREF,   .Parse = ParseUnaryOp },
    [TOKEN_AMPERSAND]               = { .precedence = 60, .type = EXPR_ADDR,    .Parse = ParseUnaryOp },
    [TOKEN_KEYWORD_SIZEOF]          = { .precedence = 60, .type = EXPR_SIZEOF,  .Parse = ParseUnaryOp },
    [TOKEN_IDENTIFIER]              = {                                         .Parse = ParseIdentifier },
    [TOKEN_LEFT_ROUND_BRACKET]      = {                                         .Parse = ParseBracket },
    [TOKEN_LITERAL_NUMBER]          = {                                         .Parse = ParseNumber },
    [TOKEN_LITERAL_STRING]          = {                                         .Parse = ParseString },
};


static struct Expr *ParseBinaryOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *rhs = ParseExpr(data.precedence - data.is_right_associative);
    return NewOperationExpr(data.type, data.lhs, rhs);
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

    // Array subscript
    if (Lexer_PeekToken(l).type == TOKEN_LEFT_SQUARE_BRACKET) {
        ExpectAndEat(TOKEN_LEFT_SQUARE_BRACKET);
        struct Expr *index = ParseExpr(0);
        ExpectAndEat(TOKEN_RIGHT_SQUARE_BRACKET);

        struct Expr *var = NewVariableExpr(identifier);
        struct Expr *add = NewOperationExpr(EXPR_ADD, var, index);
        return NewOperationExpr(EXPR_DEREF, add, NULL);
    }

    return NewVariableExpr(identifier);
}

static struct Expr *ParseNumber(struct OperatorParseData data) {
    int value = Lexer_PeekToken(l).int_value;
    ExpectAndEat(TOKEN_LITERAL_NUMBER);
    return NewNumberExpr(value);
}

static struct Expr *ParseString(struct OperatorParseData data) {
    char * value = Lexer_PeekToken(l).str_value;
    ExpectAndEat(TOKEN_LITERAL_STRING);
    return NewStringExpr(value);
}

static struct Expr *ParseUnaryOp(struct OperatorParseData data) {
    Lexer_EatToken(l);
    struct Expr *lhs = ParseExpr(data.precedence);
    return NewOperationExpr(data.type, lhs, NULL);
}

static struct AstNode *ParseDecl() {
    struct Token token = Lexer_PeekToken(l);
    struct VarDeclaration *var_declaration = NULL;

    if (token.type == TOKEN_KEYWORD_CHAR || token.type == TOKEN_KEYWORD_INT) {
        // Type
        Lexer_EatToken(l);
        var_declaration = NewVarDeclaration();
        if (token.type == TOKEN_KEYWORD_CHAR) var_declaration->type = PRIMTYPE_CHAR;
        if (token.type == TOKEN_KEYWORD_INT)  var_declaration->type = PRIMTYPE_INT;

        do {
            struct Declarator *declarator = NewDeclarator();
            List_Add(&var_declaration->declarators, declarator);

            // Pointer declarator
            while (Lexer_PeekToken(l).type == TOKEN_STAR) {
                declarator->pointer_inderection += 1;
                Lexer_EatToken(l);
            }

            // Identifier
            token = Lexer_PeekToken(l);
            strncpy(declarator->identifier, token.str_value, TOKEN_MAX_IDENTIFIER_LENGTH);
            if (Lexer_PeekToken2(l, 1).type != TOKEN_EQUALS) {
                ExpectAndEat(TOKEN_IDENTIFIER);
            }
            else {
                declarator->value = ParseExpr(0);
            }

            if (Lexer_PeekToken(l).type == TOKEN_LEFT_SQUARE_BRACKET) {
                // Array declarator
                while (Lexer_PeekToken(l).type == TOKEN_LEFT_SQUARE_BRACKET) {
                    ExpectAndEat(TOKEN_LEFT_SQUARE_BRACKET);
                    struct Token token = Lexer_PeekToken(l);
                    ExpectAndEat(TOKEN_LITERAL_NUMBER);
                    ExpectAndEat(TOKEN_RIGHT_SQUARE_BRACKET);

                    declarator->array_sizes[declarator->array_dimensions] = token.int_value;
                    declarator->array_dimensions += 1;
                }

                break;
            }
            else if (Lexer_PeekToken(l).type == TOKEN_SEMICOLON) {
                // End of declaration
                break;
            }
            else {
                ExpectAndEat(TOKEN_COMMA);
            }
        } while (true);
    }

    ExpectAndEat(TOKEN_SEMICOLON);
    return (struct AstNode *) var_declaration;
}

static struct CompoundStmt *ParseCompoundStmt() {
    struct CompoundStmt *compound_stmt = NewCompoundStmt();
    ExpectAndEat(TOKEN_LEFT_CURLY_BRACKET);
    while (Lexer_PeekToken(l).type != TOKEN_RIGHT_CURLY_BRACKET) {
        struct Token token = Lexer_PeekToken(l);
        if (token.type == TOKEN_KEYWORD_CHAR || token.type == TOKEN_KEYWORD_INT) {
            struct AstNode *decl = ParseDecl();
            List_Add(&compound_stmt->body, decl);
        }
        else {
            struct AstNode *stmt = ParseStmt();
            List_Add(&compound_stmt->body, stmt);
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

struct FunctionDef *ParseFunctionDef() {
    // Declaration specifiers
    ExpectAndEat(TOKEN_KEYWORD_INT);

    char *identifier = Lexer_PeekToken(l).str_value;
    struct FunctionDef *function = NewFunctionDef(identifier);

    ExpectAndEat(TOKEN_IDENTIFIER);
    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    while (Lexer_PeekToken(l).type != TOKEN_RIGHT_ROUND_BRACKET) {
        // Type
        ExpectAndEat(TOKEN_KEYWORD_INT);
        struct VarDeclaration *var_declaration = NewVarDeclaration();
        var_declaration->type = PRIMTYPE_INT;
        List_Add(&function->var_decls, var_declaration);

        struct Declarator *declarator = NewDeclarator();
        List_Add(&var_declaration->declarators, declarator);

        // Identifier
        struct Token token = Lexer_PeekToken(l);
        strncpy(declarator->identifier, token.str_value, TOKEN_MAX_IDENTIFIER_LENGTH);
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
        struct FunctionDef *function = ParseFunctionDef();
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

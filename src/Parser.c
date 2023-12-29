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

static struct Lexer *l;
static struct FunctionDefinition *current_function;

static void ExpectAndEat(enum TokenType type) {
    struct Token token = Lexer_PeekToken(l);
    if (token.type != type) {
        ReportErrorAtToken(l, token, "expected %d", type);
    }

    Lexer_EatToken(l);
}

static struct AstNode *ParseExpression();
static struct AstNode *ParseExpression2(int precedence);

static struct AstNode *ParseAddressOfPrefix();
static struct AstNode *ParseDereferencePrefix();
static struct AstNode *ParseLiteralNumberPrefix();
static struct AstNode *ParseNegatePrefix();
static struct AstNode *ParseParenthesisPrefix();
static struct AstNode *ParsePlusPrefix();
static struct AstNode *ParseVariablePrefix();

static struct AstNode *ParseAddInfix(struct AstNode *lhs);
static struct AstNode *ParseAssignmentInfix(struct AstNode *lhs);
static struct AstNode *ParseDivideInfix(struct AstNode *lhs);
static struct AstNode *ParseEqualsInfix(struct AstNode *lhs);
static struct AstNode *ParseGreaterThanInfix(struct AstNode *lhs);
static struct AstNode *ParseGreaterThanEqualsInfix(struct AstNode *lhs);
static struct AstNode *ParseLessThanInfix(struct AstNode *lhs);
static struct AstNode *ParseLessThanEqualsInfix(struct AstNode *lhs);
static struct AstNode *ParseMultiplyInfix(struct AstNode *lhs);
static struct AstNode *ParseNotEqualsInfix(struct AstNode *lhs);
static struct AstNode *ParseSubtractInfix(struct AstNode *lhs);

static int INFIX_OPERATOR_PRECEDENCES[TOKEN_COUNT] = {
    [TOKEN_EQUALS]                  = 10,
    [TOKEN_2_EQUALS]                = 20,
    [TOKEN_EXCLAMATION_MARK_EQUALS] = 20,
    [TOKEN_LESS_THAN]               = 30,
    [TOKEN_LESS_THAN_EQUALS]        = 30,
    [TOKEN_GREATER_THAN]            = 30,
    [TOKEN_GREATER_THAN_EQUALS]     = 30,
    [TOKEN_PLUS]                    = 40,
    [TOKEN_MINUS]                   = 40,
    [TOKEN_STAR]                    = 50,
    [TOKEN_SLASH]                   = 50,
};

static int PREFIX_OPERATOR_PRECEDENCES[TOKEN_COUNT] = {
    [TOKEN_MINUS] = 60,
    [TOKEN_STAR] = 60,
    [TOKEN_AMPERSAND] = 60,
};

typedef struct AstNode *(*PrefixParseFunction)();
static PrefixParseFunction PREFIX_PARSE_FUNCTIONS[TOKEN_COUNT] = {
    [TOKEN_LITERAL_NUMBER]      = ParseLiteralNumberPrefix,
    [TOKEN_LEFT_ROUND_BRACKET]  = ParseParenthesisPrefix,
    [TOKEN_MINUS]               = ParseNegatePrefix,
    [TOKEN_PLUS]                = ParsePlusPrefix,
    [TOKEN_IDENTIFIER]          = ParseVariablePrefix,
    [TOKEN_STAR]                = ParseDereferencePrefix,
    [TOKEN_AMPERSAND]           = ParseAddressOfPrefix,
};

typedef struct AstNode *(*InfixParseFunction)(struct AstNode *);
static InfixParseFunction INFIX_PARSE_FUNCTIONS[TOKEN_COUNT] = {
    [TOKEN_EQUALS]                  = ParseAssignmentInfix,
    [TOKEN_2_EQUALS]                = ParseEqualsInfix,
    [TOKEN_EXCLAMATION_MARK_EQUALS] = ParseNotEqualsInfix,
    [TOKEN_LESS_THAN]               = ParseLessThanInfix,
    [TOKEN_LESS_THAN_EQUALS]        = ParseLessThanEqualsInfix,
    [TOKEN_GREATER_THAN]            = ParseGreaterThanInfix,
    [TOKEN_GREATER_THAN_EQUALS]     = ParseGreaterThanEqualsInfix,
    [TOKEN_PLUS]                    = ParseAddInfix,
    [TOKEN_MINUS]                   = ParseSubtractInfix,
    [TOKEN_STAR]                    = ParseMultiplyInfix,
    [TOKEN_SLASH]                   = ParseDivideInfix,
};

static struct AstNode *ParseLiteralNumberPrefix() {
    struct Token token = Lexer_PeekToken(l);
    Lexer_EatToken(l);

    struct Literal *literal = NEW_TYPE(Literal);
    literal->node.type = AST_LITERAL_NUMBER;
    literal->int_value = token.int_value;
    literal->operand.type = OPERAND_INTEGER;
    return (struct AstNode *) literal;
}

static struct AstNode *ParseDereferencePrefix() {
    struct Token token = Lexer_PeekToken(l);
    int precedence = PREFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *expr = ParseExpression2(precedence);
    return (struct AstNode *) NewUnaryOp(OPERATOR_UNARY_DEREFERENCE, expr);
}

static struct AstNode *ParseAddressOfPrefix() {
    struct Token token = Lexer_PeekToken(l);
    int precedence = PREFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *expr = ParseExpression2(precedence);
    return (struct AstNode *) NewUnaryOp(OPERATOR_UNARY_ADDRESS_OF, expr);
}

static struct AstNode *ParseVariablePrefix() {
    struct Token token = Lexer_PeekToken(l);
    Lexer_EatToken(l);

    struct Variable *variable = NEW_TYPE(Variable);
    variable->node.type = AST_VARIABLE;
    variable->operand.type = OPERAND_INTEGER;
    strncpy(variable->identifier, token.str_value, TOKEN_MAX_IDENTIFIER_LENGTH);

    struct List *function_variables = &current_function->variables;
    struct Variable *function_variable = (struct Variable *) List_Find(function_variables, variable, AreVariablesEquals);
    if (function_variable == NULL) {
        List_Add(function_variables, (void *) variable);
    }

    return (struct AstNode *) variable;
}

static struct AstNode *ParseParenthesisPrefix() {
    Lexer_EatToken(l);
    struct AstNode *expr = ParseExpression();
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
    return expr;
}

static struct AstNode *ParseNegatePrefix() {
    struct Token token = Lexer_PeekToken(l);
    int precedence = PREFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *expr = ParseExpression2(precedence);
    return (struct AstNode *) NewUnaryOp(OPERATOR_UNARY_NEGATE, expr);
}

static struct AstNode *ParsePlusPrefix() {
    Lexer_EatToken(l);
    return ParseExpression();
}

static struct AstNode *ParseAssignmentInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence - 1); // -1 right associative
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_ASSIGN, lhs, rhs);
}

static struct AstNode *ParseEqualsInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_EQUALS, lhs, rhs);
}

static struct AstNode *ParseLessThanInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_LESS_THAN, lhs, rhs);
}

static struct AstNode *ParseLessThanEqualsInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_LESS_THAN_EQUALS, lhs, rhs);
}

static struct AstNode *ParseGreaterThanEqualsInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_GREATER_THAN_EQUALS, lhs, rhs);
}

static struct AstNode *ParseGreaterThanInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_GREATER_THAN, lhs, rhs);
}

static struct AstNode *ParseNotEqualsInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_NOT_EQUALS, lhs, rhs);
}

static struct AstNode *ParseAddInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    Lexer_EatToken(l);

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryAddOp(lhs, rhs);
}

static struct AstNode *ParseSubtractInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    Lexer_EatToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinarySubOp(lhs, rhs);
}

static struct AstNode *ParseMultiplyInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    Lexer_EatToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_MUL, lhs, rhs);
}

static struct AstNode *ParseDivideInfix(struct AstNode *lhs) {
    struct Token token = Lexer_PeekToken(l);
    Lexer_EatToken(l);
    int precedence = INFIX_OPERATOR_PRECEDENCES[token.type];

    struct AstNode *rhs = ParseExpression2(precedence);
    return (struct AstNode *) NewBinaryOp(OPERATOR_BINARY_DIV, lhs, rhs);
}

static struct AstNode *ParseExpression2(int precedence) {
    struct Token token = Lexer_PeekToken(l);
    PrefixParseFunction ParsePrefix = PREFIX_PARSE_FUNCTIONS[token.type];
    if (ParsePrefix == NULL) {
        ReportErrorAtToken(l, token, "internal error: expected expression\n");
    }

    struct AstNode *lhs = ParsePrefix();

    token = Lexer_PeekToken(l);
    int operator_precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    while (precedence < operator_precedence) {
        InfixParseFunction ParseInfix = INFIX_PARSE_FUNCTIONS[token.type];
        lhs = ParseInfix(lhs);

        token = Lexer_PeekToken(l);
        operator_precedence = INFIX_OPERATOR_PRECEDENCES[token.type];
    }

    return lhs;
}

static struct AstNode *ParseExpression() {
    return ParseExpression2(0);
}

static struct IfStatement *ParseIfStatement() {
    ExpectAndEat(TOKEN_KEYWORD_IF);
    struct IfStatement *if_statement = NEW_TYPE(IfStatement);
    if_statement->node.type = AST_IF_STATEMENT;

    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    if_statement->condition = ParseExpression();
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);

    if_statement->statement = ParseStatement();
    if (Lexer_PeekToken(l).type == TOKEN_KEYWORD_ELSE) {
        Lexer_EatToken(l);
        if_statement->else_block = ParseStatement();
    }
    else {
        if_statement->else_block = NULL;
    }

    return if_statement;
}

static struct CompoundStatement *ParseCompoundStatement() {
    ExpectAndEat(TOKEN_LEFT_CURLY_BRACKET);
    struct CompoundStatement *compound_statement = NEW_TYPE(CompoundStatement);
    compound_statement->node.type = AST_COMPOUND_STATEMENT,
    List_Init(&compound_statement->statements);
    while (Lexer_PeekToken(l).type != TOKEN_RIGHT_CURLY_BRACKET) {
        struct AstNode *statement = ParseStatement();
        List_Add(&compound_statement->statements, statement);
    }

    Lexer_EatToken(l); // TOKEN_RIGHT_CURLY_BRACKET
    return compound_statement;
}

static struct ReturnStatement *ParseReturnStatement() {
    ExpectAndEat(TOKEN_KEYWORD_RETURN);
    struct ReturnStatement *return_statement = NEW_TYPE(ReturnStatement);
    return_statement->node.type = AST_RETURN_STATEMENT;
    return_statement->expr = ParseExpression();
    ExpectAndEat(TOKEN_SEMICOLON);
    return return_statement;
}

static struct ForLoop *ParseForLoop() {
    ExpectAndEat(TOKEN_KEYWORD_FOR);
    struct ForLoop *for_loop = NEW_TYPE(ForLoop);
    for_loop->node.type = AST_FOR_LOOP;

    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    if (Lexer_PeekToken(l).type != TOKEN_SEMICOLON) {
        for_loop->init_statement = ParseExpression();
    }
    else {
        for_loop->init_statement = NULL;
    }

    ExpectAndEat(TOKEN_SEMICOLON);
    if (Lexer_PeekToken(l).type != TOKEN_SEMICOLON) {
        for_loop->condition = ParseExpression();
    }
    else {
        for_loop->condition = NULL;
    }

    ExpectAndEat(TOKEN_SEMICOLON);
    if (Lexer_PeekToken(l).type != TOKEN_RIGHT_ROUND_BRACKET) {
        for_loop->update_expr = ParseExpression();
    }
    else {
        for_loop->update_expr = NULL;
    }

    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);

    for_loop->statement = ParseStatement();
    return for_loop;
}

static struct WhileLoop *ParseWhileLoop() {
    ExpectAndEat(TOKEN_KEYWORD_WHILE);
    struct WhileLoop *while_loop = NEW_TYPE(WhileLoop);
    while_loop->node.type = AST_WHILE_LOOP;

    ExpectAndEat(TOKEN_LEFT_ROUND_BRACKET);
    while_loop->condition = ParseExpression();
    ExpectAndEat(TOKEN_RIGHT_ROUND_BRACKET);
    while_loop->statement = ParseStatement();
    return while_loop;
}

static struct AstNode *ParseStatement() {
    struct Token token = Lexer_PeekToken(l);
    switch (token.type) {
        case TOKEN_KEYWORD_FOR:         return (struct AstNode *) ParseForLoop();
        case TOKEN_KEYWORD_WHILE:       return (struct AstNode *) ParseWhileLoop();
        case TOKEN_KEYWORD_IF:          return (struct AstNode *) ParseIfStatement();
        case TOKEN_KEYWORD_RETURN:      return (struct AstNode *) ParseReturnStatement();
        case TOKEN_LEFT_CURLY_BRACKET:  return (struct AstNode *) ParseCompoundStatement();
        case TOKEN_SEMICOLON: {
            Lexer_EatToken(l);
            struct CompoundStatement *compound_statement = NEW_TYPE(CompoundStatement);
            compound_statement->node.type = AST_COMPOUND_STATEMENT;
            List_Init(&compound_statement->statements);
            return (struct AstNode *) compound_statement;
        } break;
        default: {
            struct AstNode *expr = ParseExpression();
            ExpectAndEat(TOKEN_SEMICOLON);
            return expr;
        } break;
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
        struct AstNode *statement = (struct AstNode *) ParseStatement();
        List_Add(&function->statements, statement);
    }

    return function;
}

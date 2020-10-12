#ifndef CLITTLE_PARSER_H
#define CLITTLE_PARSER_H
#include "Common.h"
#include "Lexer.h"


enum AstNodeType {
    AST_ADD, AST_SUB,
    AST_MUL, //AST_DIV,
    AST_ISEQUAL, AST_NOTEQUAL,
    AST_ISLESS_THAN, AST_ISLESS_THAN_EQUAL, AST_ISGREATER_THAN, AST_ISGREATER_THAN_EQUAL,
    AST_PRINT,

    AST_INT_LITERAL,
    AST_COMPOUND,
    AST_ASSIGN,
    AST_DECL,
    AST_IDENT,
    AST_IF,
    AST_WHILE,
};

struct AstNode {
    enum AstNodeType type;
    union {
        int intvalue;
        char *strvalue;
        struct {
            struct AstNode *lhs;
            struct AstNode *rhs;
        };
    };
};

struct AstNode *ParseCompoundStmt(struct Lexer *lexer, struct AstNode *last_statement);

#endif // CLITTLE_PARSER_H
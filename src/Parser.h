#ifndef CLITTLE_PARSER_H
#define CLITTLE_PARSER_H
#include "Common.h"
#include "Lexer.h"


enum AstNodeType {
    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_INT_LITERAL,
};

struct AstNode {
    enum AstNodeType type;
    union {
        int intvalue;
        struct {
            struct AstNode *lhs;
            struct AstNode *rhs;
        };
    };
};

struct AstNode *ParseExpr(struct Lexer *lexer, int min_precedence);
struct AstNode *ParseLiteral(struct Lexer *lexer);

#endif // CLITTLE_PARSER_H
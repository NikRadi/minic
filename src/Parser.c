#include "Parser.h"


static int op_precedence[] = {
    -1,
    -1,
    10,
    10,
    20,
    20,
    -1,
};

static enum AstNodeType GetOperatorType(enum TokenType type) {
    switch (type) {
        case TOKEN_PLUS:  return AST_ADD;
        case TOKEN_MINUS: return AST_SUB;
        case TOKEN_STAR:  return AST_MUL;
        case TOKEN_SLASH: return AST_DIV;
        default: {
            printf("invalid operator\n");
            exit(1);
        }
    }
}

static struct AstNode *AstNodeNew() {
    struct AstNode *node = (struct AstNode *) malloc(sizeof(struct AstNode));
    if (node == 0) {
        printf("failed to malloc 'AstNode'\n");
        exit(1);
    }

    return node;
}

struct AstNode *ParseExpr(struct Lexer *lexer, int min_precedence) {
    struct AstNode *lhs = ParseLiteral(lexer);
    while (TRUE) {
        ReadToken(lexer);
        int precedence = op_precedence[lexer->token.type];
        if (precedence < min_precedence) {
            break;
        }

        struct AstNode *binaryop = AstNodeNew();
        binaryop->type = GetOperatorType(lexer->token.type);
        binaryop->lhs = lhs;
        binaryop->rhs = ParseExpr(lexer, precedence);
        lhs = binaryop;
    }

    return lhs;
}

struct AstNode *ParseLiteral(struct Lexer *lexer) {
    ReadToken(lexer);
    if (lexer->token.type == TOKEN_INT_LITERAL) {
        struct AstNode *literal = AstNodeNew();
        literal->intvalue = lexer->token.intvalue;
        literal->type = AST_INT_LITERAL;
        return literal;
    }

    printf("%d: invalid literal", lexer->token.line);
    exit(1);
}

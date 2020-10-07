#include "Parser.h"


static struct AstNode *ParseLiteral(struct Lexer *lexer);

static int op_precedence[] = {
    -1, -1,
    10, 10,
    20,
    -1, -1, -1
};

static void Expect(struct Lexer *lexer, enum TokenType type, const char *str) {
    ReadToken(lexer);
    if (lexer->token.type != type) {
        printf("%d: expected '%s' (type %d) but got type %d\n",
            lexer->token.line,
            str,
            type,
            lexer->token.type
        );
        exit(1);
    }
}

static enum AstNodeType GetOperatorType(enum TokenType type) {
    switch (type) {
        case TOKEN_PLUS:  return AST_ADD;
        case TOKEN_MINUS: return AST_SUB;
        case TOKEN_STAR:  return AST_MUL;
        default: {
            printf("invalid operator of type %d\n", type);
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

static struct AstNode *ParseExpr(struct Lexer *lexer, int min_precedence) {
    struct AstNode *lhs = ParseLiteral(lexer);
    while (TRUE) {
        // TODO: Need to peek next token (not char)
        int precedence = op_precedence[lexer->peek.type];
        if (precedence < min_precedence) {
            break;
        }

        ReadToken(lexer);
        struct AstNode *binaryop = AstNodeNew();
        binaryop->type = GetOperatorType(lexer->token.type);
        binaryop->rhs = ParseExpr(lexer, precedence);
        binaryop->lhs = lhs;
        lhs = binaryop;
    }

    return lhs;
}

static struct AstNode *ParseLiteral(struct Lexer *lexer) {
    ReadToken(lexer);
    if (lexer->token.type == TOKEN_INT_LITERAL) {
        struct AstNode *literal = AstNodeNew();
        literal->intvalue = lexer->token.intvalue;
        literal->type = AST_INT_LITERAL;
        return literal;
    }

    printf("%d: invalid literal '%d'\n", lexer->token.line, lexer->token.type);
    exit(1);
}

struct AstNode *ParseStatement(struct Lexer *lexer) {
    Expect(lexer, TOKEN_PRINT, "print");
    struct AstNode *stmt = ParseExpr(lexer, 0);
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return stmt;
}

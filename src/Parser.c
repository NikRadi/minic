#include "Parser.h"


static struct AstNode *ParseLiteral(struct Lexer *lexer);

static int op_precedence[] = {
    -1, -1,
    10, 10,
    20,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
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

// TODO: expression '1*2-3+4' results in '-5' and not '3'
static struct AstNode *ParseExpr(struct Lexer *lexer, int min_precedence) {
    struct AstNode *lhs = ParseLiteral(lexer);
    while (TRUE) {
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

struct AstNode *ParsePrintStmt(struct Lexer *lexer) {
    Expect(lexer, TOKEN_PRINT, "print");
    struct AstNode *print_stmt = AstNodeNew();
    print_stmt->type = AST_PRINT;
    print_stmt->lhs = ParseExpr(lexer, 0);
    print_stmt->rhs = 0;
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return print_stmt;
}

struct AstNode *ParseVarDecl(struct Lexer *lexer) {
    Expect(lexer, TOKEN_INT, "int");
    struct AstNode *vardecl = AstNodeNew();
    vardecl->type = AST_DECL;
    vardecl->strvalue = lexer->token.strvalue;
    Expect(lexer, TOKEN_IDENTIFIER, "identifier");
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return vardecl;
}

struct AstNode *ParseVarAssign(struct Lexer *lexer) {
    struct AstNode *varassign = AstNodeNew();
    varassign->type = AST_ASSIGN;
    varassign->lhs->strvalue = lexer->peek.strvalue;
    Expect(lexer, TOKEN_IDENTIFIER, "identifier");
    Expect(lexer, TOKEN_EQUAL, "=");
    varassign->rhs = ParseExpr(lexer, 0);
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return varassign;
}

struct AstNode *ParseCompoundStmt(struct Lexer *lexer) {
    Expect(lexer, TOKEN_LEFT_CURLY_BRAC, "{");
    struct AstNode *root_compound_stmt = AstNodeNew();
    root_compound_stmt->type = AST_COMPOUND;
    root_compound_stmt->lhs = 0;
    root_compound_stmt->rhs = 0;
    struct AstNode *child_compound_stmt = root_compound_stmt;
    while (TRUE) {
        switch (lexer->peek.type) {
            case TOKEN_PRINT: {
                child_compound_stmt->lhs = ParsePrintStmt(lexer);
            } break;
            case TOKEN_INT: {
                child_compound_stmt->lhs = ParseVarDecl(lexer);
            } break;
            case TOKEN_IDENTIFIER: {
                child_compound_stmt->lhs = ParseVarAssign(lexer);
            } break;
            default: {
                printf("%d: invalid statement '%d'\n", lexer->token.line, lexer->token.type);
                exit(1);
            };
        }

        if (lexer->peek.type == TOKEN_RIGHT_CURLY_BRAC) {
            break;
        }

        child_compound_stmt->rhs = AstNodeNew();
        child_compound_stmt = child_compound_stmt->rhs;
        child_compound_stmt->lhs = 0;
        child_compound_stmt->rhs = 0;
        child_compound_stmt->type = AST_COMPOUND;
    }

    Expect(lexer, TOKEN_RIGHT_CURLY_BRAC, "}");
    return root_compound_stmt;
}

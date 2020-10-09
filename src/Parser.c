#include "Parser.h"


static struct AstNode *ParseLiteral(struct Lexer *lexer);

static int op_precedence[] = {
    0,              // EOF
    10, 10,         //  +,  -
    20, //20,       //  *,  /
    30, 30,         // ==, !=
    40, 40, 40, 40  //  <, <=, >, >=
    // Non-operators
    -1, -1, -1, -1,
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
        case TOKEN_PLUS:                return AST_ADD;
        case TOKEN_MINUS:               return AST_SUB;
        case TOKEN_STAR:                return AST_MUL;
        //case TOKEN_SLASH:             return AST_DIV;
        case TOKEN_TWO_EQUAL:           return AST_ISEQUAL;
        case TOKEN_EXMARK_EQUAL:        return AST_NOTEQUAL;
        case TOKEN_LESS_THAN:           return AST_ISLESS_THAN;
        case TOKEN_LESS_THAN_EQUAL:     return AST_ISLESS_THAN_EQUAL;
        case TOKEN_GREATER_THAN:        return AST_ISGREATER_THAN;
        case TOKEN_GREATER_THAN_EQUAL:  return AST_ISGREATER_THAN_EQUAL;
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
        literal->type = AST_INT_LITERAL;
        literal->intvalue = lexer->token.intvalue;
        return literal;
    }

    if (lexer->token.type == TOKEN_IDENT) {
        struct AstNode *ident = AstNodeNew();
        ident->type = AST_IDENT;
        ident->strvalue = lexer->token.strvalue;
        return ident;
    }

    printf("%d: invalid literal '%d'\n", lexer->token.line, lexer->token.type);
    exit(1);
}

static struct AstNode *ParsePrintStmt(struct Lexer *lexer) {
    Expect(lexer, TOKEN_PRINT, "print");
    struct AstNode *print_stmt = AstNodeNew();
    print_stmt->type = AST_PRINT;
    print_stmt->lhs = ParseExpr(lexer, 0);
    print_stmt->rhs = 0;
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return print_stmt;
}

static struct AstNode *ParseVarDecl(struct Lexer *lexer) {
    Expect(lexer, TOKEN_INT, "int");
    struct AstNode *vardecl = AstNodeNew();
    vardecl->type = AST_DECL;
    vardecl->strvalue = lexer->peek.strvalue;
    Expect(lexer, TOKEN_IDENT, "identifier");
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return vardecl;
}

static struct AstNode *ParseVarAssign(struct Lexer *lexer) {
    struct AstNode *varassign = AstNodeNew();
    varassign->type = AST_ASSIGN;
    varassign->lhs = AstNodeNew();
    varassign->lhs->strvalue = lexer->peek.strvalue;
    Expect(lexer, TOKEN_IDENT, "identifier");
    Expect(lexer, TOKEN_EQUAL, "=");
    varassign->rhs = ParseExpr(lexer, 0);
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return varassign;
}

static struct AstNode *ParseIfStmt(struct Lexer *lexer) {
    Expect(lexer, TOKEN_IF, "if");
    Expect(lexer, TOKEN_LEFT_PAREN, "(");
    struct AstNode *ifstmt = AstNodeNew();
    ifstmt->type = AST_IF;
    ifstmt->lhs = ParseExpr(lexer, 0);
    if (ifstmt->lhs->type < AST_ISEQUAL || ifstmt->lhs->type > AST_ISGREATER_THAN_EQUAL) {
        printf("invalid comparison operator\n");
        exit(1);
    }

    Expect(lexer, TOKEN_RIGHT_PAREN, ")");
    ifstmt->rhs = AstNodeNew();
    ifstmt->rhs->lhs = ParseCompoundStmt(lexer);
    if (lexer->peek.type == TOKEN_ELSE) {
        ifstmt->rhs->rhs = ParseCompoundStmt(lexer);
    }
    else {
        ifstmt->rhs->rhs = 0;
    }

    return ifstmt;
}

struct AstNode *ParseCompoundStmt(struct Lexer *lexer) {
    Expect(lexer, TOKEN_LEFT_CURLY_BRAC, "{");
    struct AstNode *root_compound_stmt = AstNodeNew();
    root_compound_stmt->type = AST_COMPOUND;
    root_compound_stmt->lhs = 0;
    root_compound_stmt->rhs = 0;
    struct AstNode *child_compound_stmt = root_compound_stmt;
    while (TRUE) {
        if (lexer->peek.type == TOKEN_RIGHT_CURLY_BRAC) {
            break;
        }

        switch (lexer->peek.type) {
            case TOKEN_PRINT: {
                child_compound_stmt->lhs = ParsePrintStmt(lexer);
            } break;
            case TOKEN_INT: {
                child_compound_stmt->lhs = ParseVarDecl(lexer);
            } break;
            case TOKEN_IDENT: {
                child_compound_stmt->lhs = ParseVarAssign(lexer);
            } break;
            case TOKEN_IF: {
                child_compound_stmt->lhs = ParseIfStmt(lexer);
            } break;
            default: {
                printf("%d: invalid statement '%d'\n", lexer->token.line, lexer->token.type);
                exit(1);
            };
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

#include "Parser.h"


static struct AstNode *ParseLiteral(struct Lexer *lexer);
static struct AstNode *ParseFuncDecl(struct Lexer *lexer);
static struct AstNode *ParseCompoundStmt(struct Lexer *lexer, struct AstNode *last_statement);


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

static struct AstNode *NewAstNode(enum AstNodeType type) {
    struct AstNode *node = (struct AstNode *) malloc(sizeof(struct AstNode));
    if (node == 0) {
        printf("failed to malloc 'AstNode'\n");
        exit(1);
    }

    node->type = type;
    return node;
}

// TODO: expression '1*2-3+4' results in '-5' and not '3'
//       '1+2-3+4' is -4 instead of 4
static struct AstNode *ParseExpr(struct Lexer *lexer, int min_precedence) {
    struct AstNode *lhs = ParseLiteral(lexer);
    while (TRUE) {
        int precedence = op_precedence[lexer->peek.type];
        if (precedence < min_precedence) {
            break;
        }

        ReadToken(lexer);
        enum AstNodeType type = GetOperatorType(lexer->token.type);
        struct AstNode *binaryop = NewAstNode(type);
        binaryop->rhs = ParseExpr(lexer, precedence);
        binaryop->lhs = lhs;
        lhs = binaryop;
    }

    return lhs;
}

static struct AstNode *ParseLiteral(struct Lexer *lexer) {
    ReadToken(lexer);
    if (lexer->token.type == TOKEN_INT_LITERAL) {
        struct AstNode *literal = NewAstNode(AST_INT_LITERAL);
        literal->intvalue = lexer->token.intvalue;
        return literal;
    }

    if (lexer->token.type == TOKEN_IDENT) {
        struct AstNode *ident = NewAstNode(AST_IDENT);
        ident->strvalue = lexer->token.strvalue;
        return ident;
    }

    printf("%d: invalid literal '%d'\n", lexer->token.line, lexer->token.type);
    exit(1);
}

static struct AstNode *ParsePrintStmt(struct Lexer *lexer) {
    Expect(lexer, TOKEN_PRINT, "print");
    struct AstNode *print_stmt = NewAstNode(AST_PRINT);
    print_stmt->lhs = ParseExpr(lexer, 0);
    print_stmt->rhs = 0;
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return print_stmt;
}

static struct AstNode *ParseVarDecl(struct Lexer *lexer) {
    Expect(lexer, TOKEN_INT, "int");
    struct AstNode *vardecl = NewAstNode(AST_DECL);
    vardecl->strvalue = lexer->peek.strvalue;
    Expect(lexer, TOKEN_IDENT, "identifier");
    Expect(lexer, TOKEN_SEMICOLON, ";");
    return vardecl;
}

static struct AstNode *ParseVarAssign(struct Lexer *lexer, enum Bool eat_semicolon) {
    struct AstNode *varassign = NewAstNode(AST_ASSIGN);
    varassign->lhs = NewAstNode(-1);
    varassign->lhs->strvalue = lexer->peek.strvalue;
    Expect(lexer, TOKEN_IDENT, "identifier");
    Expect(lexer, TOKEN_EQUAL, "=");
    varassign->rhs = ParseExpr(lexer, 0);
    if (eat_semicolon) {
        Expect(lexer, TOKEN_SEMICOLON, ";");
    }

    return varassign;
}

static struct AstNode *ParseIfStmt(struct Lexer *lexer) {
    Expect(lexer, TOKEN_IF, "if");
    Expect(lexer, TOKEN_LEFT_PAREN, "(");
    struct AstNode *ifstmt = NewAstNode(AST_IF);
    ifstmt->lhs = ParseExpr(lexer, 0);
    if (ifstmt->lhs->type < AST_ISEQUAL || ifstmt->lhs->type > AST_ISGREATER_THAN_EQUAL) {
        printf("invalid comparison operator\n");
        exit(1);
    }

    Expect(lexer, TOKEN_RIGHT_PAREN, ")");
    ifstmt->rhs = NewAstNode(-1);
    ifstmt->rhs->lhs = ParseCompoundStmt(lexer, 0);
    if (lexer->peek.type == TOKEN_ELSE) {
        ReadToken(lexer);
        ifstmt->rhs->rhs = ParseCompoundStmt(lexer, 0);
    }
    else {
        ifstmt->rhs->rhs = 0;
    }

    return ifstmt;
}

struct AstNode *ParseWhileLoop(struct Lexer *lexer) {
    Expect(lexer, TOKEN_WHILE, "while");
    Expect(lexer, TOKEN_LEFT_PAREN, "(");
    struct AstNode *whileloop = NewAstNode(AST_WHILE);
    whileloop->lhs = ParseExpr(lexer, 0);
    Expect(lexer, TOKEN_RIGHT_PAREN, ")");
    if (whileloop->lhs->type < AST_ISEQUAL || whileloop->lhs->type > AST_ISGREATER_THAN_EQUAL) {
        printf("invalid comparison operator\n");
        exit(1);
    }

    whileloop->rhs = ParseCompoundStmt(lexer, 0);
    return whileloop;
}

static struct AstNode *ParseCompoundStmt(struct Lexer *lexer, struct AstNode *last_statement) {
    Expect(lexer, TOKEN_LEFT_CURLY_BRAC, "{");
    struct AstNode *root_compound_stmt = NewAstNode(AST_COMPOUND);
    root_compound_stmt->lhs = 0;
    root_compound_stmt->rhs = 0;
    struct AstNode *child_compound_stmt = root_compound_stmt;
    while (TRUE) {
        if (lexer->peek.type == TOKEN_RIGHT_CURLY_BRAC) {
            child_compound_stmt->lhs = last_statement;
            break;
        }

        switch (lexer->peek.type) {
            case TOKEN_PRINT: {child_compound_stmt->lhs = ParsePrintStmt(lexer);} break;
            case TOKEN_IF:    {child_compound_stmt->lhs = ParseIfStmt(lexer);} break;
            case TOKEN_WHILE: {child_compound_stmt->lhs = ParseWhileLoop(lexer);} break;
            case TOKEN_INT:   {child_compound_stmt->lhs = ParseVarDecl(lexer);} break;
            case TOKEN_IDENT: {child_compound_stmt->lhs = ParseVarAssign(lexer, TRUE);} break;
            case TOKEN_FOR: {
                Expect(lexer, TOKEN_FOR, "for");
                Expect(lexer, TOKEN_LEFT_PAREN, "(");
                struct AstNode *pre_operation = ParseVarAssign(lexer, FALSE);
                Expect(lexer, TOKEN_SEMICOLON, ";");
                struct AstNode *condition = ParseExpr(lexer, 0);
                Expect(lexer, TOKEN_SEMICOLON, ";");
                struct AstNode *post_operation = ParseVarAssign(lexer, FALSE);
                Expect(lexer, TOKEN_RIGHT_PAREN, ")");
                struct AstNode *body = ParseCompoundStmt(lexer, post_operation);

                struct AstNode *whileloop = NewAstNode(AST_WHILE);
                whileloop->lhs = condition;
                whileloop->rhs = body;

                child_compound_stmt->lhs = pre_operation;

                child_compound_stmt->rhs = NewAstNode(AST_COMPOUND);
                child_compound_stmt = child_compound_stmt->rhs;
                child_compound_stmt->lhs = whileloop;
            } break;
            default: {
                printf("%d: invalid statement '%d'\n", lexer->token.line, lexer->token.type);
                exit(1);
            };
        }

        child_compound_stmt->rhs = NewAstNode(AST_COMPOUND);
        child_compound_stmt = child_compound_stmt->rhs;
        child_compound_stmt->rhs = 0;
    }

    Expect(lexer, TOKEN_RIGHT_CURLY_BRAC, "}");
    return root_compound_stmt;
}

static struct AstNode *ParseFuncDecl(struct Lexer *lexer) {
    Expect(lexer, TOKEN_VOID, "void");
    struct AstNode *funcdecl = NewAstNode(AST_FUNCTION);
    funcdecl->lhs = NewAstNode(AST_IDENT);
    Expect(lexer, TOKEN_IDENT, "identifier");
    funcdecl->lhs->strvalue = lexer->peek.strvalue;
    Expect(lexer, TOKEN_LEFT_PAREN, "(");
    Expect(lexer, TOKEN_RIGHT_PAREN, ")");
    funcdecl->rhs = ParseCompoundStmt(lexer, 0);
    return funcdecl;
}

struct AstNode *ParseFile(struct Lexer *lexer) {
    struct AstNode *ast = NewAstNode(-1);
    struct AstNode *stmt = ast;
    while (lexer->peek.type != TOKEN_EOF) {
        switch (lexer->peek.type) {
            case TOKEN_VOID: {
                stmt->lhs = ParseFuncDecl(lexer);
            } break;
            default: {
                printf("invalid global statement '%d'\n", lexer->peek.type);
                exit(1);
            } break;
        }

        stmt->rhs = NewAstNode(-1);
        stmt = stmt->rhs;
        stmt->lhs = 0;
        stmt->rhs = 0;
    }

    return ast;
}
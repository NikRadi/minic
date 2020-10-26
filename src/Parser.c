#include "Parser.h"


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
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
};


static Block *ParseBlock(Lexer *lexer);


static void ThrowError(Lexer *lexer, char *msg) {
    printf("%s(%d) error: %s\n",
        lexer->filename, lexer->token.line, msg
    );

    exit(1);
}

static void Expect(Lexer *lexer, TokenType type) {
    if (lexer->token.type != type) {
        char msg[200];
        sprintf(msg, "expected '%c' but got '%c'", type, lexer->token.type);
        ThrowError(lexer, msg);
    }
}

static OperatorType ToOperatorType(TokenType type) {
    switch (type) {
        case TOKEN_PLUS:                return OP_ADD;
        case TOKEN_MINUS:               return OP_SUB;
        case TOKEN_STAR:                return OP_MUL;
        //case TOKEN_SLASH:             return OP_DIV;
        case TOKEN_TWO_EQUAL:           return OP_ISEQUAL;
        case TOKEN_EXMARK_EQUAL:        return OP_NOTEQUAL;
        case TOKEN_LESS_THAN:           return OP_ISLESS_THAN;
        case TOKEN_LESS_THAN_EQUAL:     return OP_ISLESS_THAN_EQUAL;
        case TOKEN_GREATER_THAN:        return OP_ISGREATER_THAN;
        case TOKEN_GREATER_THAN_EQUAL:  return OP_ISGREATER_THAN_EQUAL;
        default: {
            printf("invalid operator of type %d\n", type);
            exit(1);
        }
    }
}

static Literal *ParseLiteral(Lexer *lexer) {
    if (lexer->token.type == TOKEN_INT_LITERAL) {
        Literal *literal = NEW_AST(Literal);
        literal->info.type = AST_LITERAL_INT;
        literal->intvalue = lexer->token.intvalue;
        ReadToken(lexer);
        return literal;
    }

    if (lexer->token.type == TOKEN_IDENT) {
        Literal *literal = NEW_AST(Literal);
        literal->info.type = AST_LITERAL_IDENT;
        literal->strvalue = strdup(lexer->token.strvalue);
        ReadToken(lexer);
        return literal;
    }

    ThrowError(lexer, "invalid literal ?");
    return 0; // Just to get rid of Warning C4715
}

static Ast *ParseExpr_(Lexer *lexer, int min_precedence) {
    Ast *lhs = (Ast *) ParseLiteral(lexer);
    while (op_precedence[lexer->token.type] > min_precedence) {
        int precedence = op_precedence[lexer->token.type];
        BinaryOp *binaryop = NEW_AST(BinaryOp);
        binaryop->info.type = AST_BINARYOP;
        binaryop->optype = ToOperatorType(lexer->token.type);
        ReadToken(lexer);
        binaryop->rhs = ParseExpr_(lexer, precedence);
        binaryop->lhs = lhs;
        lhs = (Ast *) binaryop;
    }

    return lhs;
}

static Ast *ParseExpr(Lexer *lexer) {
    return ParseExpr_(lexer, 0);
}

static Ast *ParseParenthesizedExpr(Lexer *lexer) {
    ReadToken(lexer);
    Expect(lexer, TOKEN_LEFT_PAREN);

    ReadToken(lexer);
    Ast *expr = ParseExpr(lexer);
    Expect(lexer, TOKEN_RIGHT_PAREN);
    return expr;
}

static VarDecl *ParseVarDecl(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_INT);
    VarDecl *vardecl = NEW_AST(VarDecl);
    vardecl->info.type = AST_VARDECL;
    vardecl->datatype = DATA_INT;
    vardecl->ident = 0;
    vardecl->expr = 0;
    ReadToken(lexer);
    Expect(lexer, TOKEN_IDENT);

    vardecl->ident = strdup(lexer->token.strvalue);
    ReadToken(lexer);
    if (lexer->token.type == TOKEN_EQUAL) {
        ReadToken(lexer);
        vardecl->expr = (Ast *) ParseExpr(lexer);
    }

    if (lexer->token.type == TOKEN_SEMICOLON) {
        ReadToken(lexer);
    }
    else {
        if (vardecl->expr == 0) {
            ThrowError(lexer, "expected ';' or '=' but got ?");
        }
        else {
            ThrowError(lexer, "expected ';' but got ?");
        }
    }

    return vardecl;
}

static VarAssign *ParseVarAssign(Lexer *lexer, Bool expect_semicolon) {
    ASSERT(lexer->token.type == TOKEN_IDENT);
    VarAssign *varassign = NEW_AST(VarAssign);
    varassign->info.type = AST_VARASSIGN;
    varassign->ident = strdup(lexer->token.strvalue);
    ReadToken(lexer);
    Expect(lexer, TOKEN_EQUAL);

    ReadToken(lexer);
    varassign->expr = ParseExpr(lexer);
    if (expect_semicolon) {
        Expect(lexer, TOKEN_SEMICOLON);
        ReadToken(lexer);
    }

    return varassign;
}

static FuncCall *ParseFuncCall(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_IDENT);
    FuncCall *funccall = NEW_AST(FuncCall);
    funccall->info.type = AST_FUNCCALL;
    funccall->ident = strdup(lexer->token.strvalue);
    funccall->arg = 0;
    ReadToken(lexer);
    Expect(lexer, TOKEN_LEFT_PAREN);
    ReadToken(lexer);
    if (lexer->token.type != TOKEN_RIGHT_PAREN) {
        funccall->arg = ParseExpr(lexer);
    }

    Expect(lexer, TOKEN_RIGHT_PAREN);
    ReadToken(lexer);
    Expect(lexer, TOKEN_SEMICOLON);
    ReadToken(lexer);
    return funccall;
}

static IfStmt *ParseIfStmt(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_IF);
    IfStmt *root_ifstmt = NEW_AST(IfStmt);
    root_ifstmt->info.type = AST_IFSTMT;
    root_ifstmt->condition = ParseParenthesizedExpr(lexer);
    root_ifstmt->block = ParseBlock(lexer);
    root_ifstmt->elsestmt = 0;
    IfStmt *child_ifstmt = root_ifstmt;
    Bool is_last_else = FALSE;
    while (lexer->token.type == TOKEN_ELSE && !is_last_else) {
        IfStmt *ifstmt = NEW_AST(IfStmt);
        ifstmt->info.type = AST_IFSTMT;
        ifstmt->condition = 0;
        if (lexer->peek.type == TOKEN_IF) {
            ReadToken(lexer);
            ifstmt->condition = ParseParenthesizedExpr(lexer);
        }
        else {
            is_last_else = TRUE;
        }

        ifstmt->block = ParseBlock(lexer);
        ifstmt->elsestmt = 0;
        child_ifstmt->elsestmt = ifstmt;
        child_ifstmt = ifstmt;
    }

    if (lexer->token.type == TOKEN_ELSE) {
        ThrowError(lexer, "else-block without a previous if-block");
    }

    return root_ifstmt;
}

static WhileLoop *ParseWhileLoop(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_WHILE);
    WhileLoop *whileloop = NEW_AST(WhileLoop);
    whileloop->info.type = AST_WHILELOOP;
    whileloop->condition = ParseParenthesizedExpr(lexer);
    whileloop->block = ParseBlock(lexer);
    return whileloop;
}

static ForLoop *ParseForLoop(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_FOR);
    ForLoop *forloop = NEW_AST(ForLoop);
    forloop->info.type = AST_FORLOOP;
    ReadToken(lexer);
    Expect(lexer, TOKEN_LEFT_PAREN);
    ReadToken(lexer);
    forloop->pre_operation = ParseVarAssign(lexer, TRUE);
    forloop->condition = ParseExpr(lexer);
    Expect(lexer, TOKEN_SEMICOLON);
    ReadToken(lexer);
    forloop->post_operation = ParseVarAssign(lexer, FALSE);
    Expect(lexer, TOKEN_RIGHT_PAREN);
    forloop->block = ParseBlock(lexer);
    return forloop;
}

static Block *ParseBlock(Lexer *lexer) {
    ReadToken(lexer);
    Expect(lexer, TOKEN_LEFT_CURLY_BRAC);

    Block *root_block = NEW_AST(Block);
    root_block->info.type = AST_BLOCK;
    root_block->info.parent = 0;
    root_block->stmt = 0;
    root_block->glue = 0;
    Block *child_block = root_block;
    ReadToken(lexer);
    while (lexer->token.type != TOKEN_RIGHT_CURLY_BRAC) {
        if (child_block->stmt != 0) {
            Block *block = NEW_AST(Block);
            block->info.type = AST_BLOCK;
            block->info.parent = (Ast *) child_block;
            block->stmt = 0;
            block->glue = 0;

            child_block->stmt->parent = (Ast *) child_block;
            child_block->glue = block;
            child_block = child_block->glue;
        }

        switch (lexer->token.type) {
            case TOKEN_INT:   {child_block->stmt = (Ast *) ParseVarDecl(lexer);} break;
            case TOKEN_IF:    {child_block->stmt = (Ast *) ParseIfStmt(lexer);} break;
            case TOKEN_WHILE: {child_block->stmt = (Ast *) ParseWhileLoop(lexer);} break;
            case TOKEN_FOR:   {child_block->stmt = (Ast *) ParseForLoop(lexer);} break;
            case TOKEN_IDENT: {
                if (lexer->peek.type == TOKEN_EQUAL) {
                    child_block->stmt = (Ast *) ParseVarAssign(lexer, TRUE);
                }
                else if (lexer->peek.type == TOKEN_LEFT_PAREN) {
                    child_block->stmt = (Ast *) ParseFuncCall(lexer);
                }
                else {
                    ThrowError(lexer, "cool error message\n");
                }
            } break;
            default: {
                printf("Test %d\n", lexer->token.type);
                exit(1);
            }
        }
    }

    ReadToken(lexer); // }
    return root_block;
}

static FuncDecl *ParseFuncDecl(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_VOID);
    FuncDecl *funcdecl = NEW_AST(FuncDecl);
    funcdecl->info.type = AST_FUNCDECL;
    funcdecl->stack_depth_bytes = 0;
    funcdecl->ident = 0;
    funcdecl->block = 0;
    ReadToken(lexer);
    Expect(lexer, TOKEN_IDENT);

    funcdecl->ident = strdup(lexer->token.strvalue);
    ReadToken(lexer);
    Expect(lexer, TOKEN_LEFT_PAREN);

    ReadToken(lexer);
    Expect(lexer, TOKEN_RIGHT_PAREN);

    funcdecl->block = ParseBlock(lexer);
    funcdecl->block->info.parent = (Ast *) funcdecl;
    return funcdecl;
}

File *ParseFile(Lexer *lexer) {
    File *root_file = NEW_AST(File);
    root_file->info.type = AST_FILE;
    root_file->funcdecl = 0;
    root_file->glue = 0;
    File *child_file = root_file;
    ReadToken(lexer);
    while (lexer->token.type != TOKEN_EOF) {
        if (child_file->funcdecl != 0) {
            File *file = NEW_AST(File);
            file->info.type = AST_FILE;
            file->funcdecl = 0;
            file->glue = 0;

            child_file->glue = file;
            child_file = child_file->glue;
        }

        switch (lexer->token.type) {
            case TOKEN_VOID: {child_file->funcdecl = ParseFuncDecl(lexer);} break;
            default: {
                ThrowError(lexer, "?");
            }
        }
    }

    return root_file;
}

#include "Parser.h"
#include "DebugPrint.h"
#include "ErrorPrint.h"


static Block *ParseBlock(Lexer *lexer);
static FuncCall *ParseFuncCall(Lexer *lexer);
static Ast *ParseExpr(Lexer *lexer);
static Ast *ParseParenthesizedExpr(Lexer *lexer);


static int op_precedences[] = {
    // do not change the order of these, OperatorTypes.def depends on it
    -1,     // not an operator
    80, 80, //  *,  /
    70, 70, //  +,  -
    60, 60, //  <, <=
    60, 60, //  >, >=
    50, 50, // ==, !=
    40,     // &&
    30,     // ||
};


static void Expect(Lexer *lexer, TokenType type) {
    if (lexer->token.type != type) {
        ThrowErrorAt(lexer,
            "expected %s but got %s",
            GetTokenTypeStr(type),
            GetTokenTypeStr(lexer->token.type)
        );
    }
}

static void ExpectAndRead(Lexer *lexer, TokenType type) {
    Expect(lexer, type);
    ReadToken(lexer);
}

static OperatorType ToOperatorType(TokenType type) {
    switch (type) {
        case TOKEN_STAR:                return OP_BIN_MUL;
        case TOKEN_SLASH:               return OP_BIN_DIV;
        case TOKEN_PLUS:                return OP_BIN_ADD;
        case TOKEN_MINUS:               return OP_BIN_SUB;
        case TOKEN_LESS_THAN:           return OP_BIN_LESS;
        case TOKEN_LESS_THAN_EQUAL:     return OP_BIN_LESS_EQUAL;
        case TOKEN_GREATER_THAN:        return OP_BIN_GREATER;
        case TOKEN_GREATER_THAN_EQUAL:  return OP_BIN_GREATER_EQUAL;
        case TOKEN_TWO_EQUAL:           return OP_BIN_ISEQUAL;
        case TOKEN_EXMARK_EQUAL:        return OP_BIN_NOTEQUAL;
        case TOKEN_TWO_AMPERSAND:       return OP_BIN_AND;
        case TOKEN_TWO_VERT_BAR:        return OP_BIN_OR;
        default:                        return 0;
    }
}

static OperatorType ToAssignmentOperatorType(TokenType type) {
    switch (type) {
        case TOKEN_EQUAL:       return OP_ASOP_EQUAL;
        case TOKEN_PLUS_EQUAL:  return OP_ASOP_ADD_EQUAL;
        case TOKEN_MINUS_EQUAL: return OP_ASOP_SUB_EQUAL;
        case TOKEN_STAR_EQUAL:  return OP_ASOP_MUL_EQUAL;
        case TOKEN_SLASH_EQUAL: return OP_ASOP_DIV_EQUAL;
        default: {
            ThrowInternalError("unexpected assignment operator '%s'\n", GetTokenTypeStr(type));
            return 0; // To get rid of warning C4715
        }
    }
}

static UnaryOp *NewUnaryOp(OperatorType optype, Ast *expr) {
    UnaryOp *unaryop = NEW(UnaryOp);
    unaryop->info.type = AST_UNARYOP;
    unaryop->optype = optype;
    unaryop->expr = expr;
    return unaryop;
}

static BinaryOp *NewBinaryOp(OperatorType optype, Ast *lhs, Ast *rhs) {
    BinaryOp *binaryop = NEW(BinaryOp);
    binaryop->info.type = AST_BINARYOP;
    binaryop->optype = optype;
    binaryop->lhs = lhs;
    binaryop->rhs = rhs;
    return binaryop;
}

static Ast *ParseLiteral(Lexer *lexer) {
    switch (lexer->token.type) {
        case TOKEN_LITERAL_INT:
        case TOKEN_LITERAL_CHAR: {
            ASSERT(TOKEN_LITERAL_INT == TOKEN_LITERAL_CHAR - 1);
            AstType asttypes[2] = {AST_LITERAL_INT, AST_LITERAL_CHAR};
            Literal *literal = NEW(Literal);
            literal->info.type = asttypes[lexer->token.type - TOKEN_LITERAL_INT];
            literal->intvalue = lexer->token.intvalue;
            ReadToken(lexer);
            return (Ast *) literal;
        }
        case TOKEN_LITERAL_STR: {
            Literal *literal = NEW(Literal);
            literal->info.type = AST_LITERAL_STR;
            literal->strvalue = lexer->token.strvalue;
            ReadToken(lexer);
            return (Ast *) literal;
        }
        case TOKEN_IDENT: {
            if (lexer->peek.type == TOKEN_LEFT_PAREN) {
                return (Ast *) ParseFuncCall(lexer);
            }

            Literal *literal = NEW(Literal);
            literal->info.type = AST_LITERAL_IDENT;
            literal->strvalue = strdup(lexer->token.strvalue);
            ReadToken(lexer); // TOKEN_IDENT
            if (lexer->token.type == TOKEN_LEFT_SQUARE_BRAC) {
                ReadToken(lexer);
                BinaryOp *binaryop = NewBinaryOp(OP_BIOP_ARR_IDX, (Ast *) literal, ParseExpr(lexer));
                ExpectAndRead(lexer, TOKEN_RIGHT_SQUARE_BRAC);
                return (Ast *) binaryop;
            }

            return (Ast *) literal;
        }
        case TOKEN_AMPERSAND: {
            ReadToken(lexer);
            return (Ast *) NewUnaryOp(OP_UN_ADDR, ParseLiteral(lexer));
        }
        case TOKEN_STAR: {
            ReadToken(lexer);
            return (Ast *) NewUnaryOp(OP_UN_DEREF, ParseLiteral(lexer));
        }
        case TOKEN_EXMARK: {
            ReadToken(lexer);
            return (Ast *) NewUnaryOp(OP_UNOP_NOT, ParseLiteral(lexer));
        }
        case TOKEN_LEFT_PAREN: {
            return ParseParenthesizedExpr(lexer);
        }
        default: {
            ThrowErrorAt(lexer, "invalid literal '%s'", GetTokenTypeStr(lexer->token.type));
            return 0; // To get rid of warning C4715
        }
    }
}

static Ast *ParseExpr_(Lexer *lexer, int min_precedence) {
    Ast *lhs = ParseLiteral(lexer);
    while (TRUE) {
        int optype = ToOperatorType(lexer->token.type);
        int op_precedence = op_precedences[optype];
        if (op_precedence <= min_precedence) {
            break;
        }

        ReadToken(lexer); // operator
        lhs = (Ast *) NewBinaryOp(optype, lhs, ParseExpr_(lexer, op_precedence));
    }

    return lhs;
}

static Ast *ParseExpr(Lexer *lexer) {
    return ParseExpr_(lexer, 0);
}

static Ast *ParseParenthesizedExpr(Lexer *lexer) {
    ExpectAndRead(lexer, TOKEN_LEFT_PAREN);
    Ast *expr = ParseExpr(lexer);
    ExpectAndRead(lexer, TOKEN_RIGHT_PAREN);
    return expr;
}

static VarDecl *ParseVarDecl(Lexer *lexer, DataType datatype) {
    ASSERT(lexer->token.type == TOKEN_INT ||
           lexer->token.type == TOKEN_CHAR ||
           lexer->token.type == TOKEN_STRUCT);
    if (datatype == DATA_STRUCT) {
        ReadToken(lexer);
    }

    ReadToken(lexer);
    VarDecl *vardecl = NEW(VarDecl);
    vardecl->info.type = AST_VARDECL;
    vardecl->ident = NULL;
    vardecl->expr = NULL;
    vardecl->lvl_indirection = 0;
    while (lexer->token.type == TOKEN_STAR) {
        ReadToken(lexer);
        vardecl->lvl_indirection += 1;
    }

    vardecl->datatype = datatype;
    Expect(lexer, TOKEN_IDENT);

    vardecl->ident = strdup(lexer->token.strvalue);
    vardecl->arrsize = -1;
    ReadToken(lexer);
    if (lexer->token.type == TOKEN_EQUAL) {
        ReadToken(lexer);
        vardecl->expr = (Ast *) ParseExpr(lexer);
    }
    else if (lexer->token.type == TOKEN_LEFT_SQUARE_BRAC) {
        ReadToken(lexer);
        vardecl->arrsize = lexer->token.intvalue;
        ReadToken(lexer);
        ExpectAndRead(lexer, TOKEN_RIGHT_SQUARE_BRAC);
    }

    return vardecl;
}

static BinaryOp *ParseVarAssign(Lexer *lexer) {
    Ast *lhs = ParseExpr(lexer);
    OperatorType optype = ToAssignmentOperatorType(lexer->token.type);
    ReadToken(lexer); // operator
    return NewBinaryOp(optype, lhs, ParseExpr(lexer));
}

static FuncCall *ParseFuncCall(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_IDENT);
    ReadToken(lexer);

    FuncCall *funccall = NEW(FuncCall);
    funccall->info.type = AST_FUNCCALL;
    funccall->ident = strdup(lexer->token.strvalue);
    funccall->args = ListNew();
    ExpectAndRead(lexer, TOKEN_LEFT_PAREN);
    if (lexer->token.type != TOKEN_RIGHT_PAREN) {
        while (TRUE) {
            Ast *arg = ParseExpr(lexer);
            ListAdd(&funccall->args, arg);
            if (lexer->token.type == TOKEN_COMMA) {
                ReadToken(lexer);
            }
            else {
                break;
            }
        }
    }

    ExpectAndRead(lexer, TOKEN_RIGHT_PAREN);
    return funccall;
}

static ReturnStmt *ParseReturnStmt(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_RETURN);
    ReadToken(lexer);

    ReturnStmt *returnstmt = NEW(ReturnStmt);
    returnstmt->info.type = AST_RETURNSTMT;
    returnstmt->expr = ParseExpr(lexer);
    ExpectAndRead(lexer, TOKEN_SEMICOLON);
    return returnstmt;
}

static IfStmt *ParseIfStmt(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_IF);
    ReadToken(lexer);

    IfStmt *root_ifstmt = NEW(IfStmt);
    root_ifstmt->info.type = AST_IFSTMT;
    root_ifstmt->condition = ParseParenthesizedExpr(lexer);
    root_ifstmt->block = ParseBlock(lexer);
    root_ifstmt->elsestmt = NULL;
    IfStmt *child_ifstmt = root_ifstmt;
    Bool is_last_else = FALSE;
    while (lexer->token.type == TOKEN_ELSE && !is_last_else) {
        IfStmt *ifstmt = NEW(IfStmt);
        ifstmt->info.type = AST_IFSTMT;
        ifstmt->condition = NULL;
        ReadToken(lexer);
        if (lexer->token.type == TOKEN_IF) {
            ReadToken(lexer);
            ifstmt->condition = ParseParenthesizedExpr(lexer);
        }
        else {
            is_last_else = TRUE;
        }

        ifstmt->block = ParseBlock(lexer);
        ifstmt->elsestmt = NULL;
        child_ifstmt->elsestmt = ifstmt;
        child_ifstmt = ifstmt;
    }

    if (lexer->token.type == TOKEN_ELSE) {
        ThrowErrorAt(lexer, "else-block without a previous if-block");
    }

    return root_ifstmt;
}

static WhileLoop *ParseWhileLoop(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_WHILE);
    ReadToken(lexer);

    WhileLoop *whileloop = NEW(WhileLoop);
    whileloop->info.type = AST_WHILELOOP;
    whileloop->condition = ParseParenthesizedExpr(lexer);
    whileloop->block = ParseBlock(lexer);
    return whileloop;
}

static ForLoop *ParseForLoop(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_FOR);
    ReadToken(lexer);

    ForLoop *forloop = NEW(ForLoop);
    forloop->info.type = AST_FORLOOP;
    ExpectAndRead(lexer, TOKEN_LEFT_PAREN);
    forloop->pre_operation = ParseVarAssign(lexer);
    ExpectAndRead(lexer, TOKEN_SEMICOLON);
    forloop->condition = ParseExpr(lexer);
    ExpectAndRead(lexer, TOKEN_SEMICOLON);
    forloop->post_operation = ParseVarAssign(lexer);
    ExpectAndRead(lexer, TOKEN_RIGHT_PAREN);
    forloop->block = ParseBlock(lexer);
    return forloop;
}

static Block *ParseBlock(Lexer *lexer) {
    ExpectAndRead(lexer, TOKEN_LEFT_CURLY_BRAC);
    Block *block = NEW(Block);
    block->info.type = AST_BLOCK;
    block->stmts = ListNew();
    while (lexer->token.type != TOKEN_RIGHT_CURLY_BRAC) {
        Ast *stmt = NULL;
        switch(lexer->token.type) {
            case TOKEN_CHAR:   {stmt = (Ast *) ParseVarDecl(lexer, DATA_CHAR); ExpectAndRead(lexer, TOKEN_SEMICOLON);} break;
            case TOKEN_INT:    {stmt = (Ast *) ParseVarDecl(lexer, DATA_INT); ExpectAndRead(lexer, TOKEN_SEMICOLON);} break;
            case TOKEN_STAR:   {stmt = (Ast *) ParseVarAssign(lexer); ExpectAndRead(lexer, TOKEN_SEMICOLON);} break;
            case TOKEN_RETURN: {stmt = (Ast *) ParseReturnStmt(lexer);} break;
            case TOKEN_IF:     {stmt = (Ast *) ParseIfStmt(lexer);} break;
            case TOKEN_WHILE:  {stmt = (Ast *) ParseWhileLoop(lexer);} break;
            case TOKEN_FOR:    {stmt = (Ast *) ParseForLoop(lexer);} break;
            case TOKEN_IDENT:  {
                if ((TOKEN_EQUAL <= lexer->peek.type && lexer->peek.type <= TOKEN_SLASH_EQUAL) ||
                        lexer->peek.type == TOKEN_LEFT_SQUARE_BRAC ||
                        lexer->peek.type == TOKEN_DOT) {
                    stmt = (Ast *) ParseVarAssign(lexer);
                    ExpectAndRead(lexer, TOKEN_SEMICOLON);
                }
                else if (lexer->peek.type == TOKEN_LEFT_PAREN) {
                    stmt = (Ast *) ParseFuncCall(lexer);
                    ExpectAndRead(lexer, TOKEN_SEMICOLON);
                }
                else {
                    ThrowErrorAt(lexer,
                        "identifier '%s' followed by invalid character '%s'\n",
                        lexer->token.strvalue,
                        GetTokenTypeStr(lexer->peek.type)
                    );
                }
            } break;
            default: {
                ThrowErrorAt(lexer,
                    "unknown statement starting with '%s'",
                    GetTokenTypeStr(lexer->token.type)
                );
            }
        }

        ASSERT(stmt != NULL);
        ListAdd(&block->stmts, stmt);
    }

    ReadToken(lexer); // TOKEN_RIGHT_CURLY_BRAC
    return block;
}

static FuncDecl *ParseFuncDecl(Lexer *lexer, DataType returntype) {
    ReadToken(lexer); // function return type
    FuncDecl *funcdecl = NEW(FuncDecl);
    funcdecl->info.type = AST_FUNCDECL;
    funcdecl->stack_depth_bytes = 0;
    funcdecl->params = ListNew();
    funcdecl->returntype = returntype;
    funcdecl->ident = NULL;
    funcdecl->block = NULL;
    Expect(lexer, TOKEN_IDENT);
    funcdecl->ident = strdup(lexer->token.strvalue);
    ReadToken(lexer);

    ExpectAndRead(lexer, TOKEN_LEFT_PAREN);
    if (lexer->token.type != TOKEN_RIGHT_PAREN) {
        DataType datatypes[2] = {DATA_INT, DATA_CHAR};
        while (TRUE) {
            DataType datatype = datatypes[lexer->token.type - TOKEN_INT];
            VarDecl *vardecl = ParseVarDecl(lexer, datatype);
            ListAdd(&funcdecl->params, (void *) vardecl);
            if (lexer->token.type == TOKEN_COMMA) {
                ReadToken(lexer);
            }
            else {
                break;
            }
        }
    }

    ExpectAndRead(lexer, TOKEN_RIGHT_PAREN);
    funcdecl->block = ParseBlock(lexer);
    return funcdecl;
}

File *ParseFile(Lexer *lexer) {
    File *file = NEW(File);
    file->info.type = AST_FILE;
    file->name = strdup(lexer->filename);
    file->decls = ListNew();
    while (lexer->token.type != TOKEN_EOF) {
        Ast *decl = NULL;
        switch (lexer->token.type) {
            case TOKEN_INT:    {decl = (Ast *) ParseFuncDecl(lexer, DATA_INT);} break;
            case TOKEN_CHAR:   {decl = (Ast *) ParseFuncDecl(lexer, DATA_CHAR);} break;
            case TOKEN_VOID:   {decl = (Ast *) ParseFuncDecl(lexer, DATA_VOID);} break;
            default: {
                ThrowErrorAt(lexer, "?");
            }
        }

        ASSERT(decl != NULL);
        ListAdd(&file->decls, decl);
    }

    return file;
}

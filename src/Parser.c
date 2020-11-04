#include "Parser.h"
#include "ErrorPrint.h"


static Block *ParseBlock(Lexer *lexer);
static FuncCall *ParseFuncCall(Lexer *lexer, Bool expect_semicolon);
static Ast *ParseExpr(Lexer *lexer);


static int op_precedences[] = {
    -1,     // OP_INVALID
    10, 10, // &&, ||
    20, 20, // ==, !=
    30, 30, //  <, <=
    30, 30, //  >, >=
    40, 40, //  +,  -
    50, 50, //  *,  /   (BIOP_ADD, BIOP_DIV)
    60, 60, //  *,  &   (UNOP_DEREF, UNOP_ADDRESS)
};


static void Expect(Lexer *lexer, TokenType type) {
    if (lexer->token.type != type) {
        ThrowError(lexer,
            "expected '%s' but got '%s'",
            GetTokenTypeStr(type),
            GetTokenTypeStr(lexer->token.type)
        );
    }
}

static OperatorType ToOperatorType(TokenType type) {
    switch (type) {
        case TOKEN_TWO_AMPERSAND:       return BIOP_AND;
        case TOKEN_TWO_VERT_BAR:        return BIOP_OR;
        case TOKEN_TWO_EQUAL:           return BIOP_ISEQUAL;
        case TOKEN_EXMARK_EQUAL:        return BIOP_NOTEQUAL;
        case TOKEN_LESS_THAN:           return BIOP_LESS;
        case TOKEN_LESS_THAN_EQUAL:     return BIOP_LESS_EQUAL;
        case TOKEN_GREATER_THAN:        return BIOP_GREATER;
        case TOKEN_GREATER_THAN_EQUAL:  return BIOP_GREATER_EQUAL;
        case TOKEN_PLUS:                return BIOP_ADD;
        case TOKEN_MINUS:               return BIOP_SUB;
        case TOKEN_STAR:                return BIOP_MUL;
        case TOKEN_SLASH:               return BIOP_DIV;
        default:                        return 0;
    }
}

static OperatorType ToAssignmentOperatorType(TokenType type) {
    switch (type) {
        case TOKEN_EQUAL:       return ASOP_EQUAL;
        case TOKEN_PLUS_EQUAL:  return ASOP_ADD_EQUAL;
        case TOKEN_MINUS_EQUAL: return ASOP_SUB_EQUAL;
        case TOKEN_STAR_EQUAL:  return ASOP_MUL_EQUAL;
        case TOKEN_SLASH_EQUAL: return ASOP_DIV_EQUAL;
        default:                return 0;
    }
}

static Ast *ParseLiteral(Lexer *lexer) {
    switch (lexer->token.type) {
        case TOKEN_LITERAL_INT:
        case TOKEN_LITERAL_CHAR: {
            ASSERT(TOKEN_LITERAL_INT == TOKEN_LITERAL_CHAR - 1);
            AstType asttypes[2] = {AST_LITERAL_INT, AST_LITERAL_CHAR};
            Literal *literal = NEW_AST(Literal);
            literal->info.type = asttypes[lexer->token.type - TOKEN_LITERAL_INT];
            literal->arridx = -1;
            literal->intvalue = lexer->token.intvalue;
            ReadToken(lexer);
            return (Ast *) literal;
        }
        case TOKEN_LITERAL_STR: {
            Literal *literal = NEW_AST(Literal);
            literal->info.type = AST_LITERAL_STR;
            literal->arridx = -1;
            literal->strvalue = lexer->token.strvalue;
            ReadToken(lexer);
            return (Ast *) literal;
        }
        case TOKEN_IDENT: {
            if (lexer->peek.type == TOKEN_LEFT_PAREN) {
                return (Ast *) ParseFuncCall(lexer, FALSE);
            }

            Literal *literal = NEW_AST(Literal);
            literal->info.type = AST_LITERAL_IDENT;
            literal->arridx = -1;
            literal->strvalue = strdup(lexer->token.strvalue);
            ReadToken(lexer); // TOKEN_IDENT
            if (lexer->token.type == TOKEN_LEFT_SQUARE_BRAC) {
                ReadToken(lexer);
                Expect(lexer, TOKEN_LITERAL_INT);
                literal->arridx = lexer->token.intvalue;
                ReadToken(lexer);
                Expect(lexer, TOKEN_RIGHT_SQUARE_BRAC);
                ReadToken(lexer);
            }

            return (Ast *) literal;
        }
        case TOKEN_AMPERSAND: {
            ReadToken(lexer);
            UnaryOp *unaryop = NEW_AST(UnaryOp);
            unaryop->info.type = AST_UNARYOP;
            unaryop->optype = UNOP_ADDRESS;
            unaryop->expr = ParseLiteral(lexer);
            return (Ast *) unaryop;
        }
        case TOKEN_STAR: {
            ReadToken(lexer);
            UnaryOp *unaryop = NEW_AST(UnaryOp);
            unaryop->info.type = AST_UNARYOP;
            unaryop->optype = UNOP_DEREF;
            unaryop->expr = ParseLiteral(lexer);
            return (Ast *) unaryop;
        }
        case TOKEN_EXMARK: {
            ReadToken(lexer);
            UnaryOp *unaryop = NEW_AST(UnaryOp);
            unaryop->info.type = AST_UNARYOP;
            unaryop->optype = UNOP_NOT;
            unaryop->expr = ParseLiteral(lexer);
            return (Ast *) unaryop;
        }
        case TOKEN_LEFT_PAREN: {
            ReadToken(lexer);
            Ast *expr = ParseExpr(lexer);
            Expect(lexer, TOKEN_RIGHT_PAREN);
            ReadToken(lexer);
            return expr;
        }
        default: {
            ThrowError(lexer, "invalid literal '%s'", GetTokenTypeStr(lexer->token.type));
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
        BinaryOp *binaryop = NEW_AST(BinaryOp);
        binaryop->info.type = AST_BINARYOP;
        binaryop->optype = optype;
        binaryop->rhs = ParseExpr_(lexer, op_precedence);
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

static VarDecl *ParseVarDecl(Lexer *lexer, DataType datatype, Bool expect_semicolon) {
    ASSERT(lexer->token.type == TOKEN_INT || lexer->token.type == TOKEN_CHAR);
    VarDecl *vardecl = NEW_AST(VarDecl);
    vardecl->info.type = AST_VARDECL;
    vardecl->ident = NULL;
    vardecl->expr = NULL;
    ReadToken(lexer);
    if (lexer->token.type == TOKEN_STAR) {
        if (datatype == DATA_INT) {
            datatype = DATA_INT_PTR;
        }
        else if (datatype == DATA_CHAR) {
            datatype = DATA_CHAR_PTR;
        }

        ReadToken(lexer);
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
        Expect(lexer, TOKEN_RIGHT_SQUARE_BRAC);
        ReadToken(lexer);
    }

    if (expect_semicolon) {
        Expect(lexer, TOKEN_SEMICOLON);
        ReadToken(lexer);
    }

    return vardecl;
}

static BinaryOp *ParseVarAssign(Lexer *lexer, Bool expect_semicolon) {
    BinaryOp *varassign = NEW_AST(BinaryOp);
    varassign->info.type = AST_BINARYOP;
    varassign->lhs = ParseExpr(lexer);
    varassign->optype = ToAssignmentOperatorType(lexer->token.type);
    ReadToken(lexer);
    varassign->rhs = ParseExpr(lexer);
    if (expect_semicolon) {
        Expect(lexer, TOKEN_SEMICOLON);
        ReadToken(lexer);
    }

    return varassign;
}

static FuncCall *ParseFuncCall(Lexer *lexer, Bool expect_semicolon) {
    ASSERT(lexer->token.type == TOKEN_IDENT);
    ReadToken(lexer); // TOKEN_IDENT
    FuncCall *funccall = NEW_AST(FuncCall);
    funccall->info.type = AST_FUNCCALL;
    funccall->ident = strdup(lexer->token.strvalue);
    funccall->arg = NULL;
    Expect(lexer, TOKEN_LEFT_PAREN);
    ReadToken(lexer);
    if (lexer->token.type != TOKEN_RIGHT_PAREN) {
        funccall->arg = ParseExpr(lexer);
    }

    Expect(lexer, TOKEN_RIGHT_PAREN);
    ReadToken(lexer);
    if (expect_semicolon) {
        Expect(lexer, TOKEN_SEMICOLON);
        ReadToken(lexer);
    }

    return funccall;
}

static ReturnStmt *ParseReturnStmt(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_RETURN);
    ReadToken(lexer); // TOKEN_RETURN
    ReturnStmt *returnstmt = NEW_AST(ReturnStmt);
    returnstmt->info.type = AST_RETURNSTMT;
    returnstmt->expr = ParseExpr(lexer);
    Expect(lexer, TOKEN_SEMICOLON);
    ReadToken(lexer);
    return returnstmt;
}

static IfStmt *ParseIfStmt(Lexer *lexer) {
    ASSERT(lexer->token.type == TOKEN_IF);
    IfStmt *root_ifstmt = NEW_AST(IfStmt);
    root_ifstmt->info.type = AST_IFSTMT;
    root_ifstmt->condition = ParseParenthesizedExpr(lexer);
    root_ifstmt->block = ParseBlock(lexer);
    root_ifstmt->elsestmt = NULL;
    IfStmt *child_ifstmt = root_ifstmt;
    Bool is_last_else = FALSE;
    while (lexer->token.type == TOKEN_ELSE && !is_last_else) {
        IfStmt *ifstmt = NEW_AST(IfStmt);
        ifstmt->info.type = AST_IFSTMT;
        ifstmt->condition = NULL;
        if (lexer->peek.type == TOKEN_IF) {
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
    // TODO: ParseBlock should be called when
    //       lexer->token.type is TOKEN_LEFT_CURLY_BRAC
    //       so there is no need to call ReadToken here
    ReadToken(lexer);
    Expect(lexer, TOKEN_LEFT_CURLY_BRAC);

    Block *root_block = NEW_AST(Block);
    root_block->info.type = AST_BLOCK;
    root_block->info.parent = NULL;
    root_block->stmt = NULL;
    root_block->glue = NULL;
    Block *child_block = root_block;
    ReadToken(lexer);
    while (lexer->token.type != TOKEN_RIGHT_CURLY_BRAC) {
        if (child_block->stmt != NULL) {
            Block *block = NEW_AST(Block);
            block->info.type = AST_BLOCK;
            block->info.parent = (Ast *) child_block;
            block->stmt = NULL;
            block->glue = NULL;

            child_block->stmt->parent = (Ast *) child_block;
            child_block->glue = block;
            child_block = child_block->glue;
        }

        switch (lexer->token.type) {
            case TOKEN_CHAR:   {child_block->stmt = (Ast *) ParseVarDecl(lexer, DATA_CHAR, TRUE);} break;
            case TOKEN_INT:    {child_block->stmt = (Ast *) ParseVarDecl(lexer, DATA_INT, TRUE);} break;
            case TOKEN_RETURN: {child_block->stmt = (Ast *) ParseReturnStmt(lexer);} break;
            case TOKEN_IF:     {child_block->stmt = (Ast *) ParseIfStmt(lexer);} break;
            case TOKEN_WHILE:  {child_block->stmt = (Ast *) ParseWhileLoop(lexer);} break;
            case TOKEN_FOR:    {child_block->stmt = (Ast *) ParseForLoop(lexer);} break;
            case TOKEN_STAR:   {child_block->stmt = (Ast *) ParseVarAssign(lexer, TRUE);} break;
            case TOKEN_IDENT:  {
                if ((TOKEN_EQUAL <= lexer->peek.type && lexer->peek.type <= TOKEN_SLASH_EQUAL) ||
                    lexer->peek.type == TOKEN_LEFT_SQUARE_BRAC) {
                    child_block->stmt = (Ast *) ParseVarAssign(lexer, TRUE);
                }
                else if (lexer->peek.type == TOKEN_LEFT_PAREN) {
                    child_block->stmt = (Ast *) ParseFuncCall(lexer, TRUE);
                }
                else {
                    ThrowError(lexer,
                        "identifier '%s' followed by invalid character '%s'\n",
                        lexer->token.strvalue,
                        GetTokenTypeStr(lexer->peek.type)
                    );
                }
            } break;
            default: {
                ThrowError(lexer, "unknown statement starting with '%s'", GetTokenTypeStr(lexer->token.type));
            }
        }

        ASSERT(child_block->stmt != NULL);
        child_block->stmt->parent = (Ast *) child_block;
    }

    ReadToken(lexer); // }
    return root_block;
}

static FuncDecl *ParseFuncDecl(Lexer *lexer, DataType returntype) {
    FuncDecl *funcdecl = NEW_AST(FuncDecl);
    funcdecl->info.type = AST_FUNCDECL;
    funcdecl->stack_depth_bytes = 0;
    funcdecl->num_params = 0;
    funcdecl->returntype = returntype;
    funcdecl->ident = NULL;
    funcdecl->block = NULL;
    ReadToken(lexer);
    Expect(lexer, TOKEN_IDENT);

    funcdecl->ident = strdup(lexer->token.strvalue);
    ReadToken(lexer);
    Expect(lexer, TOKEN_LEFT_PAREN);
    ReadToken(lexer);
    if (lexer->token.type != TOKEN_RIGHT_PAREN) {
        DataType datatypes[2] = {DATA_INT, DATA_CHAR};
        while (TRUE) {
            DataType datatype = datatypes[lexer->token.type - TOKEN_INT];
            VarDecl *vardecl = ParseVarDecl(lexer, datatype, FALSE);
            if (lexer->token.type == TOKEN_COMMA) {
                ReadToken(lexer);
            }
            else {
                break;
            }
        }
    }

    Expect(lexer, TOKEN_RIGHT_PAREN);
    funcdecl->block = ParseBlock(lexer);
    funcdecl->block->info.parent = (Ast *) funcdecl;
    return funcdecl;
}

File *ParseFile(Lexer *lexer) {
    File *root_file = NEW_AST(File);
    root_file->info.type = AST_FILE;
    root_file->funcdecl = NULL;
    root_file->glue = NULL;
    File *child_file = root_file;
    ReadToken(lexer);
    while (lexer->token.type != TOKEN_EOF) {
        if (child_file->funcdecl != NULL) {
            File *file = NEW_AST(File);
            file->info.type = AST_FILE;
            file->funcdecl = NULL;
            file->glue = NULL;

            child_file->glue = file;
            child_file = child_file->glue;
        }

        switch (lexer->token.type) {
            case TOKEN_INT:  {child_file->funcdecl = ParseFuncDecl(lexer, DATA_VOID);} break;
            case TOKEN_VOID: {child_file->funcdecl = ParseFuncDecl(lexer, DATA_INT);} break;
            case TOKEN_CHAR: {child_file->funcdecl = ParseFuncDecl(lexer, DATA_CHAR);} break;
            default: {
                ThrowError(lexer, "?");
            }
        }
    }

    return root_file;
}

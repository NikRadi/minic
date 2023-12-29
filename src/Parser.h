#ifndef MINIC_PARSER_H
#define MINIC_PARSER_H
#include "Lexer.h"
#include "List.h"
#include <stdbool.h>

struct FunctionDefinition *Parser_MakeAst(struct Lexer *lexer);

#endif // MINIC_PARSER_H

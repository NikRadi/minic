#ifndef MINIC_PARSER_H
#define MINIC_PARSER_H
#include "AstNodes.h"
#include "Common.h"
#include "Lexer.h"


struct File *ParseFile(struct Lexer *lexer);

#endif // MINIC_PARSER_H
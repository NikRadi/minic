#ifndef MINIC_DEBUGPRINT_H
#define MINIC_DEBUGPRINT_H
#include "AstNodes.h"
#include "Token.h"


char *GetTokenTypeStr(TokenType type);
char *GetAstTypeStr(AstType type);
char *GetOperatorTypeStr(OperatorType type);
void PrintToken(Token token);
void PrintFile(File *file);

#endif // MINIC_DEBUGPRINT_H
#ifndef MINIC_ERRORPRINT_H
#define MINIC_ERRORPRINT_H
#include <stdarg.h>
#include "Lexer.h"


void ThrowError(Lexer *lexer, char *format, ...);

#endif // MINIC_ERRORPRINT_H
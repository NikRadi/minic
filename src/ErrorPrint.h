#ifndef MINIC_ERRORPRINT_H
#define MINIC_ERRORPRINT_H
#include <stdarg.h>
#include "Lexer.h"


void ThrowErrorAt(Lexer *lexer, char *format, ...);
void ThrowFatalError(char *format, ...);
void ThrowInternalError(char *format, ...);

#endif // MINIC_ERRORPRINT_H
#include "ErrorPrint.h"


void ThrowError(Lexer *lexer, char *format, ...) {
    va_list arglist;
    va_start(arglist, format);
    printf("%s(%d) error: ", lexer->filename, lexer->token.line);
    vfprintf(stderr, format, arglist);
    fprintf(stderr, "\n");
    exit(1);
}
#include "ErrorPrint.h"


static void ThrowError(char *format, va_list arglist) {
    vfprintf(stderr, format, arglist);
    fprintf(stderr, "\n");
    exit(1);
}

void ThrowErrorAt(Lexer *lexer, char *format, ...) {
    printf("%s(%d) error: ", lexer->filename, lexer->token.line);
    va_list arglist;
    va_start(arglist, format);
    ThrowError(format, arglist);
}

void ThrowFatalError(char *format, ...) {
    printf("minic fatal error: ");
    va_list arglist;
    va_start(arglist, format);
    ThrowError(format, arglist);
}

void ThrowInternalError(char *format, ...) {
    printf("minic internal error: ");
    va_list arglist;
    va_start(arglist, format);
    ThrowError(format, arglist);
}
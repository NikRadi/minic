#include "ReportError.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void ReportError(char *code, char *location, char *format, va_list args) {
    int position = code - location;
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fprintf(stderr, "%s\n", code);
    fprintf(stderr, "%*s", position, "");
    fprintf(stderr, "^\n");
    exit(1);
}


//
// ===
// == Functions defined in CodeGeneratorX86.h
// ===
//


void ReportInternalError(char *format, ...) {
    va_list args;
    va_start(args, format);

    printf("internal error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    exit(1);
}

void ReportErrorAt(struct Lexer *l, char *location, char *format, ...) {
    va_list args;
    va_start(args, format);
    ReportError(l->code, location, format, args);
}

void ReportErrorAtToken(struct Lexer *l, struct Token token, char *format, ...) {
    va_list args;
    va_start(args, format);
    ReportError(l->code, token.location, format, args);
}

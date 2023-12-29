#ifndef MINIC_REPORT_ERROR_H
#define MINIC_REPORT_ERROR_H
#include "Lexer.h"

void ReportInternalError(char *format, ...);

void ReportErrorAt(struct Lexer *l, char *location, char *format, ...);

void ReportErrorAtToken(struct Lexer *l, struct Token token, char *format, ...);

#endif // MINIC_REPORT_ERROR_H

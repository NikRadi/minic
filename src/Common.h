#ifndef MINIC_COMMON_H
#define MINIC_COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable : 4127) // while (TRUE) expr is constant
#pragma warning(disable : 4201) // nameless struct/union
#pragma warning(disable : 4996) // _CRT_SECURE_NO_WARNINGS, fopen_s instead of fopen

#define NEW(type) (type *) malloc(sizeof(type));

#define ASSERT(x) \
    if(!(x)) { \
        fprintf(stderr, \
            "%s(%ld): assertion failed\n" \
            "%s\n", \
            __FILE__, __LINE__, \
            #x \
        ); \
 \
        exit(1); \
    }

enum Bool {
    FALSE, TRUE
} typedef Bool;

#endif // MINIC_COMMON_H
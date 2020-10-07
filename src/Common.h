#ifndef CLITTLE_COMMON_H
#define CLITTLE_COMMON_H
#include <stdio.h>
#include <stdlib.h>
#pragma warning(disable : 4127) // while (TRUE) expr is constant
#pragma warning(disable : 4201) // nameless struct/union
#pragma warning(disable : 4996) // _CRT_SECURE_NO_WARNINGS, fopen_s instead of fopen

enum Bool {
    FALSE, TRUE
};

#endif // CLITTLE_COMMON_H
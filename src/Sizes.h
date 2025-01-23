#ifndef MINIC_SIZES_H
#define MINIC_SIZES_H
#include "AstNode.h"

static char bytes[PRIMTYPE_COUNT] = {
    [PRIMTYPE_INVALID]  = 0,
    [PRIMTYPE_CHAR]     = 1,
    [PRIMTYPE_INT]      = 8,
    [PRIMTYPE_PTR]      = 8
};

#endif // MINIC_SIZES_H

#include "List.h"
#include <stdlib.h>
#include <string.h>


static void Reallocate(struct List *l) {
    void **new_data = (void **) malloc(sizeof(void *) * l->capacity);
    memcpy(new_data, l->data, l->count * (sizeof(void *)));
    free(l->data);
    l->data = new_data;
}


//
// ===
// == Functions defined in List.h
// ===
//


void List_Add(struct List *l, void *element) {
    if (l->count == l->capacity) {
        l->capacity <<= 2;
        Reallocate(l);
    }

    l->data[l->count] = element;
    l->count += 1;
}

void List_Free(struct List *l) {
    free(l->data);
    l->capacity = 0;
    l->count = 0;
    l->data = NULL;
}

void *List_Get(struct List *l, int index) {
    return l->data[index];
}

void List_Init(struct List *l) {
    l->capacity = 16;
    l->count = 0;
    l->data = malloc(sizeof(void *) * l->capacity);
}

#ifndef MINIC_LIST_H
#define MINIC_LIST_H
#include <stdbool.h>

typedef bool (*ListElementsEqualityFunction)(void *, void *);

struct List {
    int capacity;
    int count;
    void **data;
};

void List_Add(struct List *l, void *element);

void *List_Find(struct List *l, void *element, ListElementsEqualityFunction AreEquals);

void List_Free(struct List *l);

void *List_Get(struct List *l, int index);

void List_Init(struct List *l);

#endif // MINIC_LIST_H

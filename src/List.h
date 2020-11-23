#ifndef MINIC_LIST_H
#define MINIC_LIST_H


struct List {
    int count;
    int capacity;
    void **data;
} typedef List;


List ListNew();
void ListAdd(List *list, void *item);
void *ListGet(List *list, int idx);

#endif // MINIC_LIST_H
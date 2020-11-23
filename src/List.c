#include "List.h"
#include <stdlib.h>
#include "Common.h"


static void ReAlloc(List *list) {
    void *new_data = malloc(sizeof(void *) * list->capacity);
    memcpy(new_data, list->data, list->count);
    free(list->data);
    list->data = new_data;
}

List ListNew() {
    List list;
    list.count = 0;
    list.capacity = 4;
    list.data = malloc(sizeof(void *) * list.capacity);
    return list;
}

void ListAdd(List *list, void *item) {
    if (list->count == list->capacity) {
        list->capacity <<= 2;
        ReAlloc(list);
    }

    list->data[list->count] = item;
    list->count += 1;
}

void *ListGet(List *list, int idx) {
    ASSERT(0 <= idx && idx < list->count);
    return list->data[idx];
}
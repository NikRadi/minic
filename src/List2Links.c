#include "List2Links.h"
#include <stdlib.h>
#include "Common.h"


List2Links List2LNew() {
    List2Links list;
    list.head = NULL;
    list.tail = NULL;
    list.count = 0;
    return list;
}

void List2LAdd(List2Links *list, Ast *item) {
    Node2Links *node = (Node2Links *) malloc(sizeof(Node2Links));
    node->item = item;
    node->next = NULL;
    node->prev = list->tail;
    list->tail = node;
    if (list->head == NULL) {
        list->head = node;
        ASSERT(list->head == list->tail && list->tail == node);
    }
    else {
        node->prev->next = node;
    }

    list->count += 1;
}
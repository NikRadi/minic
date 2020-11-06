#ifndef MINIC_LIST2LINKS_H
#define MINIC_LIST2LINKS_H


struct Ast;
typedef struct Ast Ast;

struct Node2Links {
    Ast *item;
    struct Node2Links *prev;
    struct Node2Links *next;
} typedef Node2Links;

struct List2Links {
    Node2Links *head;
    Node2Links *tail;
    int count;
} typedef List2Links;


List2Links List2LNew();
void List2LAdd(List2Links *list, Ast *item);

#endif // MINIC_LIST2LINKS_H
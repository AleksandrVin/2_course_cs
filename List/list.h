/**
 * @file list.h
 * @author Aleksandr Vinogradov
 * @brief c style list generic lib
 * @version 0.1
 * @date 2020-03-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

#if malloc_err_thrower // if defined malloc will replaced with debug one

#endif

struct Node
{
    struct Node *next;
    struct Node *prev;
    void *data;
};

struct List
{
    struct Node *head;
    struct Node *tail;
    size_t size;
};

struct List *List_create();
int List_delete(struct List *list);

void *List_front(struct List *list);
void *List_back(struct List *list);

int List_push_back(struct List *list, void *data);
int List_push_front(struct List *list, void *data);

void *List_pop_back(struct List *list);
void *List_pop_front(struct List *list);

void *List_back(struct List *list);
void *List_front(struct List *list);

size_t List_first(struct List *list);
size_t List_next(struct List *list, size_t iterator);
bool List_is_end(struct List *list, size_t iterator);
void *List_get(struct List *list, size_t iterator);

/**
 * @brief insert node after node with iterator provided
 * 
 * @param list 
 * @param iterator 
 * @param data 
 * @return int 
 */
int List_insert(struct List *list, size_t iterator, void *data);
int List_remove(struct List *list, size_t iterator);

int List_size(struct List *list);

// system

struct Node *List_get_node_pointer(struct List *list, size_t iterator);

#endif
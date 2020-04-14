/**
 * @file list.c
 * @author Aleksandr Vinogradov 
 * @brief c style list generic lib
 * @version 0.1
 * @date 2020-03-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "list.h"

// malloc debugging
#ifdef malloc_debug
#define malloc(x) (malloc_err ? NULL : malloc(x))
#endif

extern bool malloc_err;

struct List *List_create()
{
    struct List *list = malloc(sizeof(struct List));
    if (list == NULL)
    {
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

int List_delete(struct List *list)
{
    if (list == NULL)
    {
        return EXIT_FAILURE;
    }
    while (List_size(list) > 0)
    {
        List_remove(list,List_first(list));
    }
    free(list);
    return EXIT_SUCCESS;
}

List_iter_t List_remove(struct List *list, List_iter_t iterator)
{
    if (list == NULL)
    {
        return NULL;
    }
    struct Node *node = List_get_node_pointer(list, iterator);
    List_iter_t iter_temp = List_next(list, iterator);
    if (node == NULL)
    {
        return NULL;
    }
    if (list->tail == node)
    {
        list->tail = node->next;
    }
    if (list->head == node)
    {
        list->head = node->prev;
    }
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    list->size--;
    free(node);
    return iter_temp;
}

void *List_front(struct List *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    if (list->size > 0)
    {
        return list->head->data;
    }
    return NULL;
}

void *List_back(struct List *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    if (list->size > 0)
    {
        return list->tail->data;
    }
    return NULL;
}

int List_push_front(struct List *list, void *data)
{
    if (list == NULL)
    {
        return EXIT_FAILURE;
    }
    struct Node *node = malloc(sizeof(struct Node));
    if (node == NULL)
    {
        return EXIT_FAILURE;
    }

    node->next = NULL;
    node->prev = NULL;
    node->data = data;
    list->size++;
    if (list->head == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        node->prev = list->head;
        list->head->next = node;
        list->head = node;
    }
    return EXIT_SUCCESS;
}
int List_push_back(struct List *list, void *data)
{
    if (list == NULL)
    {
        return EXIT_FAILURE;
    }
    struct Node *node = malloc(sizeof(struct Node));
    if (node == NULL)
    {
        return EXIT_FAILURE;
    }

    node->next = NULL;
    node->prev = NULL;
    node->data = data;
    list->size++;
    if (list->tail == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        node->next = list->tail;
        list->tail->prev = node;
        list->tail = node;
    }
    return EXIT_SUCCESS;
}

void *List_pop_front(struct List *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    if (list->tail == NULL)
    {
        return NULL;
    }
    void *data = list->head->data;
    if (list->head->prev != NULL)
    {
        list->head = list->head->prev;
        free(list->head->next);
        list->head->next = NULL;
        list->size--;
    }
    else // if only one element in list
    {
        List_remove(list, List_first(list));
    }
    return data;
}
void *List_pop_back(struct List *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    if (list->tail == NULL)
    {
        return NULL;
    }
    void *data = list->tail->data;
    List_remove(list, List_first(list));
    return data;
}

int List_insert(struct List *list, List_iter_t iterator, void *data)
{
    if (list == NULL)
    {
        return EXIT_FAILURE;
    }
    if (List_size(list) == 0)
    {
        return List_push_back(list, data);
    }
    struct Node *node = List_get_node_pointer(list, iterator);
    if (node == NULL)
    {
        return EXIT_FAILURE;
    }
    if (list->head == node)
    {
        return List_push_front(list, data);
    }
    struct Node *node_new = malloc(sizeof(struct Node));
    if (node_new == NULL)
    {
        return EXIT_FAILURE;
    }
    node_new->prev = node;
    node_new->next = node->next;
    node_new->data = data;

    node->next->prev = node_new;
    node->next = node_new;
    list->size++;
    return EXIT_SUCCESS;
}

struct Node *List_get_node_pointer(struct List *list, List_iter_t iterator)
{
    assert(list != NULL);
    return iterator;
}

void *List_get(struct List *list, List_iter_t iterator)
{
    if (iterator == NULL)
    {
        return NULL;
    }
    return iterator->data;
}

int List_size(struct List *list)
{
    if (list == NULL)
    {
        return -1;
    }
    return list->size;
}

List_iter_t List_first(struct List *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    return list->tail;
}
List_iter_t List_next(struct List *list, List_iter_t iterator)
{
    if (list == NULL || iterator == NULL)
    {
        return NULL;
    }
    return iterator->next;
}

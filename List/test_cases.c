/**
 * @file test_cases.c
 * @author Vinogradov Aleksandr 
 * @brief test for list.c
 * @version 0.1
 * @date 2020-03-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#include "list.h"

bool malloc_err = false;
#ifndef per_test_exec
#define per_test_exec false
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"


#define TEST_int(a, b)                                                                 \
    if(per_test_exec) getchar();                                                          \
    if ((a) != (b))                                                                    \
        printf(ANSI_COLOR_RED "\ttest failed: %d != %d\n" ANSI_COLOR_RESET, (a), (b)); \
    else                                                                               \
        printf(ANSI_COLOR_GREEN "test done\n" ANSI_COLOR_RESET)

#define TEST_pointer(a, b)                                                                 \
    if(per_test_exec) getchar();                                                          \
    if ((void*)(a) != (void*)(b))                                                                    \
        printf(ANSI_COLOR_RED "\ttest failed: %p != %p\n" ANSI_COLOR_RESET, (void*)(a), (void*)(b)); \
    else                                                                               \
        printf(ANSI_COLOR_GREEN "test done\n" ANSI_COLOR_RESET)

/* void print_list_int(struct List *list)
{
    struct Node *node = list->tail;
    for (size_t i = 0; i < list->size; i++)
    {
        printf("list elem N %ld is %d\n", i, *(int *)(node->data));
        node = node->next;
    }
} */

//extern magic_list_print_func(struct List *list);

int main()
{

    int data_return;
    /**
     * @brief test filling list with nums form 0 to 99 in direct order and poping them back and list size test
     * 
     */
    struct List *list_1 = List_create();
    int data[100];
    for (size_t i = 0; i < 100; i++)
    {
        data[i] = i;
        List_push_front(list_1, &(data[i]));
    }
    //print_list_int(list_1);
    //printf("list size is %d\n", List_size(list_1));
    for (size_t i = 0; i < 100; i++)
    {
        data_return = *(int *)List_pop_back(list_1);
        TEST_int(data_return, data[i]);
    }
    List_delete(list_1);

    /**
     * @brief test access to front and back elems
     * 
     */
    list_1 = List_create();
    for (size_t i = 10; i <= 50; i++)
    {
        List_push_back(list_1, &(data[i]));
    }
    data_return = *(int *)List_back(list_1);
    TEST_int(data_return, data[50]);

    data_return = *(int *)List_front(list_1);
    TEST_int(data_return, data[10]);
    List_delete(list_1);

    /**
     * @brief test pop front func 
     * 
     */
    list_1 = List_create();
    for (int i = 0; i < 50; i++)
    {
        data[i] = i;
        List_push_front(list_1, &(data[i]));
    }
    //print_list_int(list_1);
    for (int i = 49; i >= 0; i--)
    {
        data_return = *(int *)List_pop_front(list_1);
        TEST_int(data_return, data[i]);
    }
    List_delete(list_1);

    /**
 * @brief test get elem by iter and list next func test
 * 
 */
    list_1 = List_create();
    for (int i = 0; i < 10; i++)
    {
        data[i] = i;
        List_push_front(list_1, &(data[i]));
    }
    List_iter_t iter = List_first(list_1); // list[0] elem
    iter = List_next(list_1, iter);   // list[1] elem
    iter = List_next(list_1, iter);   // list[2] elem
    data_return = *(int *)List_get(list_1, iter);
    TEST_int(data_return, data[2]);
    List_delete(list_1);

    /**
 * @brief delete not empty list
 * 
 */
    list_1 = List_create();
    for (int i = 0; i < 10; i++)
    {
        data[i] = i;
        List_push_front(list_1, &(data[i]));
    }
    List_delete(list_1);

    /**
     * @brief list insert and list remove test
     * 
     */
    list_1 = List_create();
    for (int i = 0; i < 10; i++)
    {
        data[i] = i;
        List_push_front(list_1, &(data[i]));
    }
    iter = List_first(list_1);
    iter = List_next(list_1, iter); // list[1] elem
    iter = List_next(list_1, iter); // list[2] elem
    List_insert(list_1, iter, &(data[10]));

    iter = List_next(list_1, iter);
    data_return = *(int *)List_get(list_1, iter);
    TEST_int(data_return, data[10]);

    iter = List_remove(list_1, iter);
    data_return = *(int *)List_get(list_1, iter);
    TEST_int(data_return, data[3]);
    List_delete(list_1);

    /**
     * @brief malloc error test while creating 
     * 
     */
    malloc_err = true;
    list_1 = List_create();
    TEST_pointer(list_1, NULL);
    malloc_err = false;

    list_1 = List_create();
    malloc_err = true;
    TEST_int(List_push_front(list_1,data), EXIT_FAILURE);
    TEST_int(List_push_back(list_1,data),EXIT_FAILURE);
    
    malloc_err = false; // to add two elems
    List_push_back(list_1,data);
    List_push_front(list_1,&data[2]);
    iter = List_first(list_1);
    malloc_err = true;
    TEST_int(List_insert(list_1,List_first(list_1),&data[1]),EXIT_FAILURE);
    malloc_err = false;
    List_delete(list_1);



    /**
     * @brief tests for input list pointer == NULL
     * 
     */
    int return_value = List_delete(NULL);
    TEST_int(return_value, EXIT_FAILURE);

    List_iter_t return_value_2 = List_remove(NULL, List_first(NULL));
    TEST_pointer(return_value_2, NULL);

    void *return_value_p = List_front(NULL);
    TEST_pointer(return_value_p, NULL);

    return_value_p = List_back(NULL);
    TEST_pointer(return_value_p, NULL);

    return_value = List_push_back(NULL, NULL);
    TEST_int(return_value, EXIT_FAILURE);

    return_value = List_push_front(NULL, NULL);
    TEST_int(return_value, EXIT_FAILURE);

    return_value_p = List_pop_back(NULL);
    TEST_pointer(return_value_p, NULL);

    return_value_p = List_pop_front(NULL);
    TEST_pointer(return_value_p, NULL);

    return_value = List_insert(NULL, List_first(NULL), NULL);
    TEST_int(return_value, EXIT_FAILURE);

    return_value = List_size(NULL);
    TEST_int(return_value, -1);

    /**
     * @brief parse wrong iterator to funcs
     * 
     */
    list_1 = List_create();
    int value = 1;
    List_push_front(list_1,&value);
    iter = List_first(list_1);
    iter = List_next(list_1,iter);
    TEST_pointer(List_remove(list_1,iter), NULL);
    TEST_int(List_insert(list_1,iter,data),EXIT_FAILURE);

    List_delete(list_1);

    /**
     * @brief test where list is empty 
     * 
     */
    list_1 = List_create();
    TEST_pointer(List_front(list_1),NULL);
    TEST_pointer(List_back(list_1),NULL);
    TEST_pointer(List_pop_front(list_1),NULL);
    TEST_pointer(List_pop_back(list_1),NULL);
    List_delete(list_1);

    /**
     * @brief list insert when node is head or tail
     * 
     */
    list_1 = List_create();
    List_push_back(list_1,&data[0]);
    List_push_front(list_1,&(data[2]));
    iter = List_first(list_1);
    List_insert(list_1,iter,&(data[1]));
    iter = List_first(list_1);
    iter = List_next(list_1,iter);
    iter = List_next(list_1,iter); // last elem == head
    List_insert(list_1,iter,&data[3]);
    value = *(int*)List_pop_front(list_1);
    TEST_int(value,data[3]);
    value = *(int*)List_pop_front(list_1);
    TEST_int(value,data[2]);
    value = *(int*)List_pop_front(list_1);
    TEST_int(value,data[1]);
    value = *(int*)List_pop_front(list_1);
    TEST_int(value,data[0]);

    TEST_int((int)List_size(list_1), 0);
    List_delete(list_1);

    /**
     * @brief insert to empty list
     * 
     */
    list_1 = List_create();
    List_insert(list_1,List_first(list_1),data);
    TEST_pointer(List_pop_front(list_1),data);
    List_delete(list_1);

    /**
     * @brief list_get from item NULL
     * 
     */
    list_1 = List_create();
    TEST_pointer(List_get(list_1, NULL), NULL);
    List_delete(list_1);

    return EXIT_SUCCESS;
}
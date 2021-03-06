/**
 * @file 3_thread.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief Creates threads for increemnt number. Show error in threads work with variable
 * @version 0.1
 * @date 2019-09-13
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>

#include <pthread.h>

#include "../Custom_libs/Lib_funcs.h"

// max amount of threads using by program
#define THREAD_AMOUNTS_MAX 35

/**
 * @brief Test thread pointer to function to increment argument
 * 
 * @param number pointer to number to increment 
 * @return int 0 or error code
 */

long long int number_thread = 0;

void *thread_increment(void *number)
{
    // how much every thread must increment number
    for (size_t i = 0; i < *((int*)number); i++)
    {
        number_thread += 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        printf("to many parameters\n");
        return -1;
    }
    if (argc == 1)
    {
        printf("provide number N as argument\n");
        return -1;
    }
    if (argc != 2)
    {
        printf("unknow argument number error\n");
        return -1;
    }

    long long int number = ReadPosNumberFromArg(1, argv);
    // increment for last thread of in main func
    long long int number_for_last_thread = number % THREAD_AMOUNTS_MAX;

    // variable for threads

    if (number < 0)
    {
        printf("error in number format, can't read\n");
        return -1;
    }

    // array with thread id
    pthread_t tids[THREAD_AMOUNTS_MAX];

    // load default attrebuts in attr of thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    long long int number_per_thread = number / THREAD_AMOUNTS_MAX;

    // creates THREAD_AMOUNT_MAX thread
    for (size_t i = 0; i < THREAD_AMOUNTS_MAX; i++)
    {
        int status = pthread_create(&(tids[i]), &attr, thread_increment, &number_per_thread);
        if (status != 0)
        {
            printf("#### Error in thread creating happend\n");
            return -1;
        }
    }

    for (size_t i = 0; i < THREAD_AMOUNTS_MAX; i++)
    {
        //int ptrval;
        int status = pthread_join(tids[i], NULL);
        /*         if (*(int *)ptrval != 0)
        {
            printf("thread_increment function failed ot operate in thread number: %ld\n", i);
            return -1;
        } */
        if (status != 0)
        {
            printf("#### Error in thread creating happend\n");
            return -1;
        }
    }

    // add remainder for number_thread after thread done their work
    number_thread += number_for_last_thread;

    printf("Number after thread have finished work is equal to %lld\n", number_thread);
    printf("Expected value of number is %lld\n", number);
    return 0;
}
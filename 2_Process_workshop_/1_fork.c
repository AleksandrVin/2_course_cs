/**
 * @file main.c
 * @author Aleksandr Vinogradov
 * @brief task 1 of second practice lesson 
 * prints pin of process and order of its exec
 * @version 0.1
 * @date 2019-09-13
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>

// includes for PID
#include <sys/types.h>
#include <unistd.h>

#include "../Lib_funcs.h"

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

    long long int number = (size_t)ReadPosNumberFromArg(1, argv);
    if (number < 0)
    {
        printf("error in number format, can't read\n");
        return -1;
    }

    for (size_t i = 0; i < number; i++)
    {
        pid_t pid = fork();

// for parent 
        if(pid > 0)
        {
            printf("Parent forked child with pid = %d\n", pid);
            return 0;
        }
        else if(pid != 0)
        {
            printf("#### error in fork happened ####\n");
            return -1;
        }

// for child 
        if (pid == 0)
        {
            pid_t local_pid = getpid();
            printf("\tPid of this child is %d\n", local_pid);
            printf("\t\tthis child was forked %ld in turn\n", i);
            // close process
            exit(0);
        }

    }
}
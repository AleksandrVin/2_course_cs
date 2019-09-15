/**
 * @file 2_exec.c
 * @author Aleksandr Vinogradov
 * @brief execute program with args
 * @version 0.1
 * @date 2019-09-13
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>

// for exec funcs
#include <unistd.h>

#define PATH_ARG 1

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("provide program name and it's args as argument\n");
        return -1;
    }
    if (argc < 2)
    {
        printf("unknow argument number error\n");
        return -1;
    }

    // change process 
    // TODO do errno stuff for exexvp
    execvp(argv[PATH_ARG], argv + 1);
    printf("process wasn't launched. error\n");
    return -1;
}
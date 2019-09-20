/**
 * @file Lib_funcs.c
 * @author Aleksandr Vinogradov
 * @brief Some userfull funcs for tasks
 * @version 0.1
 * @date 2019-09-13
 * 
 * @copyright Copyright (c) 2019
 * 
 */



#ifndef LIB_FUNCS
#define LIB_FUNCS

#include <stdlib.h>
#include <errno.h>


// TODO print to stderr and set ernno
/**
 * @brief func for reading unsigned int number from arg and prints error to stdout 
 * 
 * @param argv_number number of argument to read
 * @param argv argv from main func
 * @return long long int -1 if error or number
 */
long long int ReadPosNumberFromArg(unsigned int argv_number, char **argv)
{
    // check pointer to string
    if (argv[argv_number] == NULL)
    {
        printf("bad pointer to input string\n");
        return -1;
    }

    char *ptr_for_last_char = NULL;
    unsigned long long int number = strtoull(argv[argv_number], &ptr_for_last_char, 10);

    if (*ptr_for_last_char != '\0')
    {
        printf("this is not number with base 10\n");
        return -1;
    }

    if (errno == ERANGE)
    {
        printf("input number out of range or incorrect\n");
        return -1;
    }

    if (number < 1)
    {
        printf("number if smaller then 1\n");
        return -1;
    }

    return number;
}

#endif /* LIB_FUNCS */
/**
 * @file main.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief Firts practice task of 2 course mipt CS
 * @version 0.1
 * @date 2019-09-06
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#define INPUT_N_STRING argv[1]

/**
 * @brief print to stdout numbers from 1 to number
 * 1 <= number <= __LONG_LONG_MAX__
 * 
 * @param number 
 * @return 0 if succes or -1 if error
 */
int PrintNumber(unsigned long long int number);

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

    // check pointer to string
    if (INPUT_N_STRING == NULL)
    {
        printf("bad pointer to input string\n");
        return -1;
    }


 /*    char * copy_pointer_of_number_char = INPUT_N_STRING;
    while(*copy_pointer_of_number_char != '\0')
    {
        if(!isdigit(*copy_pointer_of_number_char))
        {
            printf("only numbers accepted\n");
            return -1;
        }
        copy_pointer_of_number_char++;
    } */

    char * ptr_for_last_char = NULL;
    unsigned long long int number = strtoull(INPUT_N_STRING, &ptr_for_last_char, 10);

    if( *ptr_for_last_char != '\0')
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

    return PrintNumber(number);
}

int PrintNumber(unsigned long long int number)
{
    for (long long int i = 1; i <= number; i++)
    {
        if (printf("%lld ", i) <= 0)
        {
            printf("printing error\n");
            return -1;
        }
    }
    printf("\n");
    return 0;
}
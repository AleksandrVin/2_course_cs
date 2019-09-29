/**
 * @file file_transfer.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief File transfer between console in one OS
 * @version 0.1
 * @date 2019-09-23
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// error codes
#include <errno.h>

#include "../Custom_libs/Lib_funcs.h"

#define ARG_ID 2
#define ARG_MODE 1

typedef unsigned int id_t;

const char * mode_send = "-send";
const char * mode_receive = "-receive"; 

int main(int argc, char ** argv)
{
    //TODO rewrite it with normal style of arg parsing
    // check input params
    if(argc != 3 && argc != 4)
    {
        err_printf("Bad args\n");
        return EXIT_FAILURE;
    }
    if(argc == 3) // mode receive
    {
        // check mode
        if(strcmp(argv[ARG_MODE], mode_receive) != 0)
        {
            err_printf("Bad mode\n");
            return EXIT_FAILURE;
        }

        id_t id = ReadPosNumberFromArg(ARG_ID,argv);
        if(id < 1)
        {
            err_printf("ID isn't correct");
            return -1;
        }
        if(id == 0)
        {
            err_printf("ID must be above zero");
            return -1;
        }
        //TODO continue here ( replace printf )
        printf("id = %d\n", id);
        

    }
    return 0;
}
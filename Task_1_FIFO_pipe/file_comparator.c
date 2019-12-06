/**
 * @file file_comparator.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief func to compare two open and valid FD and return true, is fd points to the exactly one file
 * @version 0.1
 * @date 2019-12-01
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>

// for open call ( man 2 open )
#include <fcntl.h>

// for stat call ( man 2 stat )
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../Custom_libs/Lib_funcs.h"

int compare_files( int file_1 , int file_2);

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        err_printf("BAD ARGC \n \tuse ./[program] file1 file2 \n");
        return EXIT_FAILURE;
    }
    
    int file_1 = open(argv[1], O_RDONLY);
    if( file_1 < 0)
    {
        perror("problem with first file");
        return EXIT_FAILURE;
    }
    
    int file_2 = open(argv[2], O_RDONLY);
    if( file_2 < 0)
    {
        perror("problem with second file");
        return EXIT_FAILURE;
    }

    compare_files(file_1 , file_2) ? printf("these are one files\n") : printf("these are diffent files\n");
    return EXIT_SUCCESS;
}

int compare_files( int file_1 , int file_2)
{
    struct stat sb_1;
    struct stat sb_2;

    if(fstat(file_1, &sb_1) == -1)
    {
        perror("fstat error file_1");
        exit(EXIT_FAILURE);
    }

    if(fstat(file_2, &sb_2) == -1)
    {
        perror("fstat error file_2");
        exit(EXIT_FAILURE);
    }

    if( sb_1.st_dev == sb_2.st_dev && sb_1.st_ino == sb_2.st_ino)
    {
        return 1;
    }
    return 0;
}
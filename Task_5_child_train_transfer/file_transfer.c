/**
 * @file file_transfer.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief File transfer between console in one OS using SIGUSR1 and SIGUSR2 bit by bit sending
 * @version 0.1
 * @date 2019-12-6
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#define _GNU_SOURCE     // for semtimedop();
#define _POSIX_C_SOURCE // for sigaction();

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <time.h> // for timespec

#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>

#include "../Custom_libs/Lib_funcs.h"

#define BUFF_SIZE 1024

#define PIPE_READ_FD 0
#define PIPE_WRITE_FD 1

#define WAIT_TIMEOUT 3

// buff size is ( BUFF_CALC_BASE^(child_amount - child_num)*BUFF_CALC_MULTIPLIER )
#define BUFF_MAX_SIZE 0x20000 //  128k byte
#define BUFF_CALC_BASE 3
#define BUFF_CALC_MULTIPLIER 0x400 // 1k byte

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

struct child_stuff
{
    int fd_in;
    int fd_out;
    int buff_size;
    int child_num;
};

int ChildFunc(struct child_stuff child_stuff_complete);
int Buff_CalculateSize(int child_amount, int child_num);

void my_sigchld_handler(int nsig);

int main(int argc, char **argv)
{
    // check args
    if (argc == 1)
    {
        err_printf("provide number N as argument and file name\n");
        return EXIT_FAILURE;
    }
    if (argc != 3)
    {
        err_printf("to many argc\n");
        return EXIT_FAILURE;
    }

    // read number of child from args
    int child_amount = (int)ReadPosNumberFromArg(1, argv);
    if (child_amount < 0)
    {
        err_printf("error in number format, can't read\n");
        return -1;
    }

    // open file to print
    int fd_of_file = open(argv[2], O_RDONLY);
    if (fd_of_file == -1)
    {
        perror("Error in opening file to read");
        return EXIT_FAILURE;
    }

    // set handler for SIGCHLD
    if(signal(SIGCHLD, my_sigchld_handler) == SIG_ERR)
    {
        perror("can't set handler for SIGCHLD\n");
        exit(EXIT_FAILURE);
    }

    int pipefd[2];
    int pipefd_old = fd_of_file; // confusing algorithm like programming competition
    for (size_t i = 0; i < child_amount; i++)
    {
        // set child struct
        struct child_stuff child_stuff_complete;

        if (i != (child_amount - 1))
        {
            if (pipe(pipefd) != 0)
            {
                perror("can't create pipe for child\n");
                exit(EXIT_FAILURE);
            }
        }

        child_stuff_complete.fd_in = pipefd_old;

        if (i == (child_amount - 1))
        {
            child_stuff_complete.fd_out = 1; // stdout
        }
        else
        {
            child_stuff_complete.fd_out = pipefd[PIPE_WRITE_FD];
        }

        child_stuff_complete.buff_size = Buff_CalculateSize(child_amount, i);
        child_stuff_complete.child_num = i;

        pid_t pid = fork();

        // for parent
        if (pid > 0)
        {
            //printf("Parent forked child with pid = %d\n", pid);
            // close all pipefd in parent fork
            if (i != (child_amount - 1))
            {

                if (close(pipefd_old) == -1 && close(pipefd[PIPE_WRITE_FD] != 1))
                {
                    perror("can't close fd in parent 1");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (close(pipefd_old) == -1)
                {
                    perror("can't close fd in parent 2");
                    exit(EXIT_FAILURE);
                }
            }
            pipefd_old = pipefd[PIPE_READ_FD];
        }
        else if (pid != 0)
        {
            perror("#### error in fork happened ####\n");
            return EXIT_FAILURE;
        }

        // for child
        if (pid == 0)
        {
            pid_t local_pid = getpid();
            printf("\tPid of this child is %d and i'm %lu\n", local_pid, i);
            ChildFunc(child_stuff_complete);
            // close process
        }
    }

    return pause();
}

/**
 * @brief function for childs
 * 
 * @param key_sem NO_SEM if no sem used, key of sem value if this is the first child in train 
 * @return int 
 */
int ChildFunc(struct child_stuff child_stuff_complete)
{
    /* fd_set rfds;
    struct timeval tv;
    int retval;

    // watch for pipe_in to see when it has input
    FD_ZERO(&rfds);
    FD_SET(child_stuff_complete.fd_in, &rfds);

    // wait for WAIT_TIMEOUT
    tv.tv_sec = WAIT_TIMEOUT;
    tv.tv_usec = 0;

    retval = select(child_stuff_complete.fd_in + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
        perror("error in select in child\n");
        exit(EXIT_FAILURE);
    }
    else if (retval == 0)
    {
        perror("select timeout in child || ");
        err_printf("child number %d with pid %d and buff size is %d\n", child_stuff_complete.child_num, getpid(), child_stuff_complete.buff_size);
        exit(EXIT_FAILURE);
    }

    // reading from sender
    char *buff = (char *)calloc(child_stuff_complete.buff_size, sizeof(char)); */
    char buff[BUFF_MAX_SIZE];
    if (buff < 0)
    {
        perror("can't calloc buff in child\n");
        exit(EXIT_FAILURE);
    }

    int read_amount = 0;
    int write_amount = 0;
    do
    {
        read_amount = read(child_stuff_complete.fd_in, buff, BUFF_MAX_SIZE);
        if (read_amount < 0)
        {
            perror("Error in reading occurred int child\n");
            exit(EXIT_FAILURE);
        }

        write_amount = write(child_stuff_complete.fd_out, buff, read_amount);
        //err_printf(ANSI_COLOR_GREEN "wrote %d bytes form %d\n" ANSI_COLOR_RESET, write_amount, read_amount);
        while (write_amount < read_amount)
        {
            int new_write_amount = write(child_stuff_complete.fd_out, buff + write_amount, read_amount - write_amount);
            if (write_amount < 0)
            {
                perror("error in writing occurred in child\n");
                exit(EXIT_FAILURE);
            }
            write_amount += new_write_amount;
        }
        if (write_amount < 0)
        {
            perror("error in writing occurred in child\n");
            exit(EXIT_FAILURE);
        }
    } while (read_amount > 0);
    
    exit(EXIT_SUCCESS);
}

/**
 * @brief calculate size of buffer according to the task 5
 * 
 * @param child_num number of child in train
 * @return int 
 */
int Buff_CalculateSize(int child_amount, int child_num)
{
    if (child_num < 0)
    {
        err_printf("error in Buff_CalculateSize\n");
        return -1;
    }
    double buff_size = pow(BUFF_CALC_BASE, child_amount - 1 - child_num) * BUFF_CALC_MULTIPLIER;
    if (buff_size > BUFF_MAX_SIZE)
    {
        return BUFF_MAX_SIZE;
    }
    return (int)buff_size;
}

void my_sigchld_handler(int nsig)
{
    pid_t w;
    int wstatus;
    w = wait(&wstatus);
    if (w == -1)
    {
        perror("wait error\n");
        exit(EXIT_FAILURE);
    }
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == EXIT_FAILURE)
    {
        err_printf(ANSI_COLOR_RED "child exit with EXIT_FAILURE\n" ANSI_COLOR_RESET);
        exit(EXIT_FAILURE);
    }
    else if (WIFSIGNALED(wstatus))
    {
        err_printf(ANSI_COLOR_RED "child killed by signal %d\n" ANSI_COLOR_RESET, WTERMSIG(wstatus));
        exit(EXIT_FAILURE);
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}
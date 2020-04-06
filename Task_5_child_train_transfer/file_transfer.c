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

#include "../Custom_libs/Lib_funcs.h"

#define BUFF_SIZE 1024

#define PIPE_READ_FD 0
#define PIPE_WRITE_FD 1

#define WAIT_TIMEOUT 3

// buff size is ( BUFF_CALC_BASE^(child_amount - child_num)*BUFF_CALC_MULTIPLIER )
#define BUFF_MAX_SIZE 0x20000 //  128k byte
#define BUFF_CALC_BASE 3
#define BUFF_CALC_MULTIPLIER 0x400 // 1k byte

#define BUFF_CHILD 512

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
};

struct parent_staff // amount of parent stuff buff is child_stuff - 1
{
    // fd for child
    int fd_in;
    int fd_out;

    char *buf;
    char *cur_read;
    int amount_in_buff;
    int size;
};

int buff_get_free_space_for_linear_write(const struct parent_staff *parent_data);
int buff_get_amount_for_linear_read(const struct parent_staff *parent_data);

int Buff_CalculateSize(int child_amount, int child_num);

int ChildFunc(int n,const struct child_stuff child_stuff_complete);

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

    if(child_amount > 510)
    {
        err_printf("amount of children to much\n");
        exit(EXIT_FAILURE);
    }

    // open file to print
    int fd_of_file = open(argv[2], O_RDONLY);
    if (fd_of_file == -1)
    {
        perror("Error in opening file to read");
        return EXIT_FAILURE;
    }

    // crate buff for parent
    struct parent_staff *buff_parent_staff = (struct parent_staff *)calloc(child_amount - 1, sizeof(*buff_parent_staff));
    if (buff_parent_staff < 0)
    {
        perror("can't calloc parent buff\n");
        exit(EXIT_FAILURE);
    }

    int pipefd_1[2];
    int pipefd_2[2];
    for (size_t i = 0; i < child_amount; i++)
    {
        // set child struct
        struct child_stuff child_stuff_complete;

        if (i != 0) // if not first
        {
            if (pipe(pipefd_1) != 0)
            {
                perror("can't create pipe for child\n");
                exit(EXIT_FAILURE);
            }
            child_stuff_complete.fd_in = pipefd_1[PIPE_READ_FD];
            buff_parent_staff[i - 1].fd_out = pipefd_1[PIPE_WRITE_FD];
           // err_printf(ANSI_COLOR_CYAN "pipe 1 for %lu" ANSI_COLOR_RESET, i);
        }
        else // if first
        {
            child_stuff_complete.fd_in = fd_of_file;
        }

        if (i != (child_amount - 1)) // if not last
        {
            if (pipe(pipefd_2) != 0)
            {
                perror("can't create pipe for child\n");
                exit(EXIT_FAILURE);
            }
            buff_parent_staff[i].fd_in = pipefd_2[PIPE_READ_FD];
            child_stuff_complete.fd_out = pipefd_2[PIPE_WRITE_FD];
            //err_printf(ANSI_COLOR_CYAN "pipe 2 for %lu" ANSI_COLOR_RESET, i);
        }
        else
        {
            child_stuff_complete.fd_out = 1; // stdout
        }

        pid_t pid = fork();

        // for parent
        if (pid > 0)
        {
            //printf("Parent forked child with pid = %d\n", pid);
            // close all pipefd in parent fork
            if (i == 0 && i == (child_amount - 1))
            {
                if (close(fd_of_file) == -1)
                {
                    perror("can't close file in parent\n");
                    exit(EXIT_FAILURE);
                }
               // err_printf(ANSI_COLOR_MAGENTA "fd of file close for %lu\n" ANSI_COLOR_RESET, i);
            }
            else if (i == 0)
            {
                if (close(pipefd_2[PIPE_WRITE_FD]) == -1)
                {
                    perror("can't close fd for last child in parent\n");
                    exit(EXIT_FAILURE);
                }
                //err_printf(ANSI_COLOR_MAGENTA "pipe 2 closed write for %lu\n" ANSI_COLOR_RESET, i);
            }
            else if (i == (child_amount - 1)) // if not last
            {
                if (close(pipefd_1[PIPE_READ_FD] == -1))
                {
                    perror("can't close fd in parent for first fd\n");
                    exit(EXIT_FAILURE);
                }
                //err_printf(ANSI_COLOR_MAGENTA "pipe 1 closed read for %lu\n" ANSI_COLOR_RESET, i);
            }
            else
            {
                if (close(pipefd_1[PIPE_READ_FD]) == -1 || close(pipefd_2[PIPE_WRITE_FD]) == -1)
                {
                    perror("can't close fd in parent\n");
                    exit(EXIT_FAILURE);
                }
                //err_printf(ANSI_COLOR_MAGENTA "pipe 1 closed for read and 2 for write for %lu\n" ANSI_COLOR_RESET, i);
            }
        }
        else if (pid != 0)
        {
            perror("#### error in fork happened ####\n");
            return EXIT_FAILURE;
        }

        // for child
        if (pid == 0)
        {
            for (size_t j = 0; j <= i; j++)
            {
                if (i == 0 && i == (child_amount - 1)) // if first and last
                {
                }
                else if (i == 0) // if first
                {
                    if (close(buff_parent_staff[j].fd_in) == -1)
                    {
                        perror("can't close fd_2 read in child\n");
                        exit(EXIT_FAILURE);
                    }
                    //err_printf(ANSI_COLOR_RED "pipe 2 closed for read child %lu" ANSI_COLOR_RESET, i);
                }
                else if (i == (child_amount - 1)) // if last
                {
                    if (close(buff_parent_staff[j - 1].fd_out) == -1)
                    {
                        perror("can't close fd_2 read in child\n");
                        exit(EXIT_FAILURE);
                    }
                    //err_printf(ANSI_COLOR_RED "pipe 1 closed for write child %lu" ANSI_COLOR_RESET, i);
                }
                else // if not first and not last
                {
                    if (close(buff_parent_staff[j - 1].fd_out) == -1 || close(buff_parent_staff[j].fd_in) == -1)
                    {
                        perror("can't close fd in child\n");
                        exit(EXIT_FAILURE);
                    }
                    //err_printf(ANSI_COLOR_RED "pipe 1 closed for write and 2 for read child %lu" ANSI_COLOR_RESET, i);
                }
            }

            //pid_t local_pid = getpid();
            //printf("\tPid of this child is %d and i'm %lu\n", local_pid, i);
            return ChildFunc(i, child_stuff_complete);
            // close process
        }
    }

    for (size_t i = 0; i < child_amount - 1; i++)
    {
        buff_parent_staff[i].size = Buff_CalculateSize(child_amount, i);
        buff_parent_staff[i].buf = (char *)calloc(buff_parent_staff[i].size, sizeof(char));
        if (buff_parent_staff[i].buf < 0)
        {
            perror("can't calloc mem for buff in parent\n");
            exit(EXIT_FAILURE);
        }
        fcntl(buff_parent_staff[i].fd_out, F_SETFL, O_NONBLOCK);
        fcntl(buff_parent_staff[i].fd_in, F_SETFL, O_NONBLOCK);
        buff_parent_staff[i].amount_in_buff = 0;
        buff_parent_staff[i].cur_read = buff_parent_staff[i].buf;
    }

    // TODO main algorithm ( crazy stuff )
    fd_set rfds;
    fd_set wfds;
    int first_to_exit_child = 0; // child are waiting to exit first. If exit another child -> error. If this child -> wait for next child to exit
    // if first child had exited, that means that sending is over. So we need to wait to all data transfer through other childs.

    err_printf("parent pid: %d\n", getpid());

    if (child_amount == 1)
    {
        return EXIT_SUCCESS;
    }

    do
    {
        /* code */
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        // select fd for wait for read
        int max_fd = 0;

        for (size_t i = first_to_exit_child; i < child_amount - 1; i++)
        {
            if (buff_parent_staff[i].amount_in_buff == 0) // buff is empty
            {
                //err_printf("set to wait for read %lu", i);
                FD_SET(buff_parent_staff[i].fd_in, &rfds);
                if (buff_parent_staff[i].fd_in > max_fd)
                {
                    max_fd = buff_parent_staff[i].fd_in;
                }
            }
            else // if data present
            {
                //err_printf("set to wait for write %lu", i);
                FD_SET(buff_parent_staff[i].fd_out, &wfds);
                if (buff_parent_staff[i].fd_out > max_fd)
                {
                    max_fd = buff_parent_staff[i].fd_out;
                }
            }
        }
        // wait for read or write available
        if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) <= 0)
        {
            perror("error in select\n");
            exit(EXIT_FAILURE);
        }
        //err_printf("select done\n");
        // perform reading operations from fifo, nonblock
        for (size_t i = 0; i < child_amount - 1; i++)
        {
            if (FD_ISSET(buff_parent_staff[i].fd_in, &rfds))
            {
                //err_printf("reading %lu\n", i);
                int free_space_in_buff = buff_parent_staff[i].size; //  buff_get_free_space_for_linear_write((&(buff_parent_staff[i])));
                int read_amount = read(buff_parent_staff[i].fd_in, buff_parent_staff[i].buf, free_space_in_buff);
                if (read_amount == 0) // child exit
                {
                    if (i == first_to_exit_child)
                    {
                        first_to_exit_child++;
                        /* if (close(buff_parent_staff[i].fd_in) == -1)
                            {
                                perror("can't close fd to for read\n");
                                exit(EXIT_FAILURE);
                            } */
                        //err_printf(ANSI_COLOR_MAGENTA "++" ANSI_COLOR_RESET);
                    }
                    else
                    {
                        //err_printf("non first child had exited\n");
                        exit(EXIT_FAILURE);
                    }
                }
                if (read_amount < 0)
                {
                    perror("error in read in paren\n");
                    exit(EXIT_FAILURE);
                }

                // set ring buff
                buff_parent_staff[i].amount_in_buff = read_amount;
                //err_printf("exit_space in buff: %d and read_amount: %d\n", free_space_in_buff, read_amount);
                //err_printf("amount for linear read is %d\n", buff_parent_staff[i].amount_in_buff);
            }
        }
        for (size_t i = 0; i < child_amount - 1; i++)
        {
            if (FD_ISSET(buff_parent_staff[i].fd_out, &wfds))
            {
                //err_printf("writing %lu\n", i);
                int amount_to_read = buff_parent_staff[i].amount_in_buff; //  buff_get_amount_for_linear_read(&(buff_parent_staff[i]));

                if (amount_to_read > 0)
                {
                    int write_amount = write(buff_parent_staff[i].fd_out, buff_parent_staff[i].cur_read, amount_to_read);
                    if (write_amount <= 0)
                    {
                        perror("error in write to child in parent\n");
                        exit(EXIT_FAILURE);
                    }
                    if (write_amount < amount_to_read)
                    {
                        buff_parent_staff[i].cur_read += write_amount;
                        buff_parent_staff[i].amount_in_buff -= write_amount;
                    }
                    else
                    {
                        buff_parent_staff[i].cur_read = buff_parent_staff[i].buf;;
                        buff_parent_staff[i].amount_in_buff = 0;
                    }

                    //err_printf("writted %d", write_amount);
                }
                else
                {
                    err_printf("no to write\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (first_to_exit_child > 0)
        {
            if (buff_parent_staff[first_to_exit_child - 1].amount_in_buff == 0)
            {
                if (close(buff_parent_staff[first_to_exit_child - 1].fd_out))
                {
                    //perror("can't close fd out from parent\n");
                    //exit(EXIT_FAILURE);
                }
                //err_printf(ANSI_COLOR_GREEN "fd out closed %d" ANSI_COLOR_RESET, first_to_exit_child - 1);
            }
        }
        //err_printf("fist to exit is: %lu and child_amount is: %d and linera read is: %d \n", first_to_exit_child, child_amount, buff_get_amount_for_linear_read(&(buff_parent_staff[child_amount - 2])));
    } while (!(first_to_exit_child == (child_amount - 1) && buff_parent_staff[child_amount - 2].amount_in_buff == 0)); // last child waiting for exit and nothig to write

    // free buffs
    for (size_t i = 0; i < child_amount - 1; i++)
    {
        free(buff_parent_staff[i].buf);
    }
    free(buff_parent_staff);

    

    //err_printf("\nexiting\n ");
    return EXIT_SUCCESS;
}

/**
 * @brief function for childs
 * 
 * @param key_sem NO_SEM if no sem used, key of sem value if this is the first child in train 
 * @return int 
 */
int ChildFunc(int n,const struct child_stuff child_stuff_complete)
{
    char buff[BUFF_CHILD];

    int read_amount = 0;
    int write_amount = 0;
    do
    {
        read_amount = read(child_stuff_complete.fd_in, buff, BUFF_CHILD);
        if (read_amount < 0)
        {
            perror("Error in reading occurred int child\n");
            exit(EXIT_FAILURE);
        }

        /* if( n % 2 == 0)
        {
            sleep(2);
        } */

        write_amount = write(child_stuff_complete.fd_out, buff, read_amount);
        if (write_amount < 0)
        {
            perror("error in writing occurred in child\n");
            exit(EXIT_FAILURE);
        }

        if (write_amount != read_amount)
        {
            perror("child can't write all data\n");
            exit(EXIT_FAILURE);
        }
    } while (read_amount > 0);

    //err_printf(ANSI_COLOR_YELLOW "child exit" ANSI_COLOR_RESET);

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
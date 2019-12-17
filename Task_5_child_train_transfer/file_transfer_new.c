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

#define BUFF_CHILD 0x20000

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

    // circular buffer( ring buffer )
    // buff and queue variables  \\  |************************0000000000000000000000000000000000| where ****** - data , 00000 - free space
    //                               | - buff(start_pos)     | - buff + amount_in_buff         | - buff + buff_size
    // buff can be like this     \\  |0000000000000000000000000000***************000000000000000|
    //                                                           | - start_pos  | - start_pos + amount_in_buff
    // buff is organized like queue, but multiple byte per read|write allowed
    // this is used to optimise memory usage of buff, because simple one read per write can't utilize large buff
    char *buff;
    int buff_size;
    int *head;
    int *tail;
};

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

    // crate buff for parent
    struct parent_staff *buff_parent_staff = (struct parent_staff *)calloc(child_amount - 1, sizeof(*buff_parent_staff));
    if (buff_parent_staff < 0)
    {
        perror("can't calloc parent buff\n");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < child_amount - 1; i++)
    {
        buff_parent_staff[i].buff_size = Buff_CalculateSize(child_amount, i);
        buff_parent_staff[i].buff = (char *)calloc(buff_parent_staff[i].buff_size, sizeof(char));
        if (buff_parent_staff[i].buff < 0)
        {
            perror("can't calloc mem for buff in parent\n");
            exit(EXIT_FAILURE);
        }
        buff_parent_staff[i].tail = buff_parent_staff[i].buff;
        buff_parent_staff[i].head = buff_parent_staff[i].buff;
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
            if (i != (child_amount - 1)) // if not last
            {
                if (i != 0) // if not last and first
                {
                    if (close(pipefd_1[PIPE_READ_FD]) == -1 || close(pipefd_2[PIPE_WRITE_FD]) == -1)
                    {
                        perror("can't close fd in parent\n");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (close(pipefd_2[PIPE_WRITE_FD] == -1))
                    {
                        perror("can't close fd in parent for first fd\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                if (close(pipefd_1[PIPE_READ_FD]) == -1)
                {
                    perror("can't close fd for last child in parent\n");
                    exit(EXIT_FAILURE);
                }
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
            if (close(pipefd_1[PIPE_WRITE_FD]) == -1 || close(pipefd_2[PIPE_READ_FD]) == -1)
            {
                perror("can't close fd in child\n");
                exit(EXIT_FAILURE);
            }
            pid_t local_pid = getpid();
            printf("\tPid of this child is %d and i'm %lu\n", local_pid, i);
            ChildFunc(child_stuff_complete);
            // close process
        }
    }
    // close(file to read)
    if (close(fd_of_file) == -1)
    {
        perror("can't close file in parent\n");
        exit(EXIT_FAILURE);
    }

    // TODO main algorithm ( crazy stuff )
    fd_set rfds;
    fd_set wfds;
    size_t first_to_exit_child = 0; // child are waiting to exit first. If exit another child -> error. If this child -> wait for next child to exit
    // if first child had exited, that means that sending is over. So we need to wait to all data transfer through other childs.
    do
    {
        /* code */
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        // select fd for wait for read
        int max_fd = 0;

        int condition_for_read_from_child = 0;
        if (first_to_exit_child != 0)
            condition_for_read_from_child = first_to_exit_child - 1;

        for (size_t i = condition_for_read_from_child; i < child_amount - 1; i++)
        {
            if (buff_get_free_space_for_linear_write(&(buff_parent_staff[i]))) // if there is space in buff
            {
                FD_SET(buff_parent_staff[i].fd_in, &rfds);
                if (buff_parent_staff[i].fd_in > max_fd)
                {
                    max_fd = buff_parent_staff[i].fd_in;
                }
            }
        }
        for (size_t i = first_to_exit_child; i < child_amount - 1; i++)
        {
            if (buff_get_amount_for_linear_read(&(buff_parent_staff[i]))) // if there is data in buff
            {
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
        // perform reading operations from fifo, nonblock
        for (size_t i = 0; i < child_amount - 1; i++)
        {
            if (FD_ISSET(buff_parent_staff[i].fd_in, &rfds))
            {
                int free_space_in_buff = buff_get_free_space_for_linear_write((&(buff_parent_staff[i])));
                if (free_space_in_buff == 0)
                {
                    err_printf("error in free size calc\n");
                    exit(EXIT_FAILURE);
                }
                else if (free_space_in_buff > 0)
                {
                    int read_amount = read(buff_parent_staff[i].fd_in, buff_parent_staff[i].head, free_space_in_buff);
                    if (read_amount == 0) // child exit
                    {
                        if (i == first_to_exit_child)
                        {
                            first_to_exit_child++;
                        }
                        else
                        {
                            err_printf("non first child had exited\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (read_amount < 0)
                    {
                        perror("error in read in paren\n");
                        exit(EXIT_FAILURE);
                    }

                    // set ring buff
                    buff_parent_staff[i].head += read_amount;
                }
                else
                {
                    int read_amount = read(buff_parent_staff[i].fd_in, buff_parent_staff[i].buff, -free_space_in_buff);
                    if (read_amount == 0) // child exit
                    {
                        if (i == first_to_exit_child)
                        {
                            first_to_exit_child++;
                        }
                        else
                        {
                            err_printf("non first child had exited\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (read_amount < 0)
                    {
                        perror("error in read in paren\n");
                    }

                    // set ring buff
                    buff_parent_staff[i].head = buff_parent_staff[i].buff + read_amount;
                }
            }
        }
        for (size_t i = 0; i < child_amount - 1; i++)
        {
            if (FD_ISSET(buff_parent_staff[i].fd_out, &wfds))
            {
                int amount_to_read = buff_get_amount_for_linear_read(&(buff_parent_staff[i]));
                if (amount_to_read == 0)
                {
                    err_printf("error in calc space for read\n");
                    exit(EXIT_FAILURE);
                }
                else if (amount_to_read > 0)
                {
                    int write_amount = write(buff_parent_staff[i].fd_out, buff_parent_staff[i].tail, amount_to_read);
                    if (amount_to_read <= 0)
                    {
                        perror("error in write to child in parent\n");
                        exit(EXIT_FAILURE);
                    }

                    // set ring buff
                    buff_parent_staff[i].tail += write_amount;
                }
                else
                {
                    int write_amount = write(buff_parent_staff[i].fd_out, buff_parent_staff[i].buff, amount_to_read);
                    if (amount_to_read <= 0)
                    {
                        perror("error in write to child in parent 2");
                        exit(EXIT_FAILURE);
                    }
                    
                    // set ring buff
                    buff_parent_staff[i].tail = buff_parent_staff[i].buff + write_amount;
                }
            }
        }
        while (first_to_exit_child == child_amount && buff_get_amount_for_linear_read(&(buff_parent_staff[child_amount - 1])) == 0)
            ; // last child waiting for exit and nothig to write

        free(buff_parent_staff);
        return EXIT_SUCCESS;
    }

    /**
 * @brief calc free space in ring buff for linear write
 * 
 * @param parent_data 
 * @return int ( -N if write available from start for N bytes or number of bytes in write from head, which can be written after head pointer )
 */
    int buff_get_free_space_for_linear_write(const struct parent_staff *parent_data) // get how many bytes can be written to ring buffer after head pointer
    {
        size_t space = parent_data->head - parent_data->tail;
        if (space >= 0)
        {
            if (space > parent_data->buff_size)
            {
                err_printf("error in buffer");
                exit(EXIT_FAILURE);
            }
            if (parent_data->head == parent_data->buff + parent_data->buff_size)
            {
                return (-(parent_data->tail - parent_data->buff)); // write from start
            }
            return parent_data->buff + parent_data->buff_size - parent_data->head; // write from head
        }
        else
        {
            return space; // write after head
        }
    }

    /**
 * @brief calc space for liner read from ring buff
 * 
 * @param parent_data 
 * @return int ( -N if read from start of buff or N if read from tail)
 */
    int buff_get_amount_for_linear_read(const struct parent_staff *parent_data)
    {
        size_t space = parent_data->head - parent_data->tail;
        if (space >= 0)
        {
            if (space > parent_data->buff_size)
            {
                err_printf("error in buffer");
                exit(EXIT_FAILURE);
            }
            return space;
        }
        else
        {
            if (parent_data->tail == parent_data->buff + parent_data->buff_size)
            {
                return (-(parent_data->head - parent_data->buff));
            }
            return parent_data->buff + parent_data->buff_size - parent_data->tail;
        }
    }

    /**
 * @brief function for childs
 * 
 * @param key_sem NO_SEM if no sem used, key of sem value if this is the first child in train 
 * @return int 
 */
    int ChildFunc(const struct child_stuff child_stuff_complete)
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
        char buff[BUFF_CHILD];
        if (buff < 0)
        {
            perror("can't calloc buff in child\n");
            exit(EXIT_FAILURE);
        }

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
            //err_printf(ANSI_COLOR_GREEN "wrote %d bytes form %d\n" ANSI_COLOR_RESET, write_amount, read_amount);
            /* while (write_amount < read_amount)
        {
            int new_write_amount = write(child_stuff_complete.fd_out, buff + write_amount, read_amount - write_amount);
            if (write_amount < 0)
            {
                perror("error in writing occurred in child\n");
                exit(EXIT_FAILURE);
            }
            write_amount += new_write_amount;
        } */
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
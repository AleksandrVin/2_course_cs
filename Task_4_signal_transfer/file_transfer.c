/**
 * @file file_transfer.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief File transfer between parent and child in one OS using SIGUSR1 and SIGUSR2 bit by bit sending
 * @version 0.1
 * @date 2019-12-6
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#define _GNU_SOURCE // for semtimedop();

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/prctl.h>

#include <time.h> // for timespec

#include <sys/ipc.h>
#include <sys/sem.h>

#include "../Custom_libs/Lib_funcs.h"

char bit_shared = 0;

void zero(int signum)
{
    bit_shared = 0;
}

void one(int signum)
{
    bit_shared = 1;
}

void done(int signum)
{
    exit(EXIT_SUCCESS);
}

void do_nothing(int signum){};

int main(int argc, char **argv)
{
    // check args
    if (argc == 1)
    {
        err_printf("provide number N as argument and file name\n");
        return EXIT_FAILURE;
    }
    if (argc != 2)
    {
        err_printf("to many argc\n");
        return EXIT_FAILURE;
    }

    // open file to print
    int fd_of_file = open(argv[1], O_RDONLY);
    if (fd_of_file == -1)
    {
        perror("Error in opening file to read");
        return EXIT_FAILURE;
    }

    sigset_t blocked, released;
    sigemptyset(&blocked);
    sigemptyset(&released);
    sigaddset(&blocked, SIGUSR1);
    sigaddset(&blocked, SIGUSR2);
    sigprocmask(SIG_BLOCK, &blocked, &released);

    struct sigaction sig_unset, sig_set, sig_child, sig_msg;
    sig_unset.sa_handler = zero;
    sig_set.sa_handler = one;
    sig_child.sa_handler = done;
    sig_msg.sa_handler = do_nothing;
    sig_unset.sa_flags = SA_RESTART;
    sig_set.sa_flags = SA_RESTART;
    sig_child.sa_flags = SA_RESTART;
    sig_msg.sa_flags = SA_RESTART;

    sigaction(SIGUSR1, &sig_unset, NULL);
    sigaction(SIGUSR2, &sig_set, NULL);
    sigaction(SIGCHLD, &sig_child, NULL);
    sigaction(SIGCONT, &sig_msg, NULL);

    pid_t parent_pid = getpid();
    pid_t pid_fork = fork();

    if (pid_fork > 0) // parent
    {
        while (true)
        {
            char buff = 0;
            for (size_t i = 0; i < 8; i++)
            {
                sigsuspend(&released);
                buff = (buff << 1) | bit_shared;
                kill(pid_fork, SIGCONT);
            }

            if (write(1, &buff, 1) != 1)
            {
                perror("can't write\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else if (pid_fork != 0) // child
    {
        perror("fork\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        prctl(PR_SET_PDEATHSIG, SIGCHLD); // let sigchld if parent dies

        if (getppid() != parent_pid) // original parent
        {
            perror("parent already died\n");
            exit(EXIT_FAILURE);
        }

        sigset_t blocked_IO, released_IO;
        sigemptyset(&blocked_IO);
        sigaddset(&blocked_IO, SIGCONT);
        sigprocmask(SIG_BLOCK, &blocked_IO, &released_IO);

        char buff = 0;
        while (read(fd_of_file, &buff, 1) > 0)
        {
            for (size_t i = 0; i < 8; i++)
            {
                char bit_s = (buff & 0b10000000) >> 7;

                if (bit_s == 0)
                {
                    if(kill(parent_pid, SIGUSR1) != 0)
                    {
                        perror("kill child 1");
                        exit(EXIT_FAILURE);
                    }
                }
                {
                    if(kill(parent_pid, SIGUSR2) != 0)
                    {
                        perror("kill child 2\n");
                        exit(EXIT_FAILURE);
                    }
                }

                buff = buff << 1;

                sigsuspend(&released);
            }
        }

        exit(EXIT_SUCCESS);
    }
}
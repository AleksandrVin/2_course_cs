/**
 * @file 1_OpenFile_and_print.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief Open file in child process and print it in parent
 * @version 0.1
 * @date 2019-09-20
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>

// for pipe
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFF_SIZE 1024

#define READ_FD 0
#define WRITE_FD 1

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        printf("to many parameters\n");
        return -1;
    }
    if (argc == 1)
    {
        printf("provide file name as argument\n");
        return -1;
    }
    if (argc != 2)
    {
        printf("unknow argument number error\n");
        return -1;
    }

    int pipefd[2];

    if (pipe(pipefd))
    {
        printf("error in pipe creating happened\n");
        return -1;
    }

    int pid = fork();

    // for child
    if (pid == 0)
    {
        //pid_t local_pid = getpid();
        //printf("\tPid of this child is %d\n", local_pid);

        close(pipefd[READ_FD]);
        int file_for_read = open(argv[1], O_RDONLY);
        char buff[BUFF_SIZE];
        int read_amount = 0;
        do
        {
            read_amount = read(file_for_read, buff, BUFF_SIZE);
            write(pipefd[WRITE_FD], buff, read_amount);
        } while (read_amount > 0);
        close(file_for_read);
        return 0;
    }

    // for parent
    if (pid > 0)
    {
        //printf("Parent forked child with pid = %d\n", pid);

        close(pipefd[WRITE_FD]);
        char buff[BUFF_SIZE];
        int read_amount = 0;
        do
        {
            read_amount = read(pipefd[READ_FD], buff, BUFF_SIZE);
            if (read_amount == 0)
            {
                //printf("\n END \n");
                return 0;
            }
            write(1, buff, read_amount);
        } while (read_amount > 0);
        return 0;
    }
    else if (pid != 0)
    {
        printf("#### error in fork happened ####\n");
        return -1;
    }

    printf("some thing goes wrong");
    return -1;
}
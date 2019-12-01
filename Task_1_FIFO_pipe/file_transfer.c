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

#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
// error codes
#include <errno.h>

#include <limits.h>

#include <dirent.h>

#include "../Custom_libs/Lib_funcs.h"

#define ARG_MODE 1
#define ARG_NAME 2

#define FIFO_NAME_SIZE 100

#define WAIT_TIMEOUT 3
#define SEND_ATTEMPTS 5

#define START_CODE_SIZE 1

const char start_code[START_CODE_SIZE] = {'~'};

#define BUFF_SIZE 4096 // size of buffer used to send file with write/read

#define ID_LENGTH 22 // 5 for "/tmp/" , 11 for 4 bite unsigned int , 5 for ".fifo" , and 1 for "\0"

const char *mode_send = "-send";
const char *mode_receive = "-receive";

int ReceiveFile();
int SendFile(const char *file_name);
void PrepareName(char *buff_out, pid_t pid);
int FindFifo(char *name);
int WaitForSender(int fd);

const char dir_for_fifo[] = "/tmp/";
const char file_name_type[] = ".fifo";
const char fifo_name[] = "fifo_alek";

const char fifo_global_name[] = "/tmp/global_process_fifo.fifo";

int main(int argc, char **argv)
{
    // check input params

    if (argc != 2 && argc != 3)
    {
        err_printf("Bad args\n");
        return EXIT_FAILURE;
    }
    if (argc == 2) // mode receive
    {
        // check mode
        if (strcmp(argv[ARG_MODE], mode_receive) != 0)
        {
            err_printf("Bad mode\n");
            return EXIT_FAILURE;
        }

        // id text is ARG_ID
        return ReceiveFile();
    }
    if (argc == 3)
    {
        // check mode
        if (strcmp(argv[ARG_MODE], mode_send) != 0)
        {
            err_printf("Bad mode\n");
            return EXIT_FAILURE;
        }

        return SendFile(argv[ARG_NAME]);
    }
    err_printf("something went bad in main arg parsing\n");
    return EXIT_FAILURE;
}

/**
 * @brief Init connection and read data from FIFO
 * 
 * @param id 
 * @return int 
 */
int ReceiveFile()
{

    // private fifo for data transfer
    char name[FIFO_NAME_SIZE] = "";
    PrepareName(name, getpid());
    int fd_private = open(name, O_RDONLY | O_NDELAY); // fd of FIFO

    if (fd_private < 0)
    {
        umask(0); // set mask to 0000
        if (mkfifo(name, 0600) != 0)
        {
            perror(NULL);
            err_printf("can't create FIFO_private\n");
            close(fd_private);
            exit(EXIT_FAILURE);
        }
        fd_private = open(name, O_RDONLY | O_NDELAY);
        if (fd_private < 0)
        {
            err_printf("can't open FIFO_private\n");
            unlink(name);
            close(fd_private);
            exit(EXIT_FAILURE);
        }
    }

    // child process waits for parent
    // global fifo for synchronization
    int fd_global = open(fifo_global_name, O_RDWR | O_NDELAY); // fd of FIFO

    if (fd_global < 0)
    {
        umask(0); // set mask to 0000
        if (mkfifo(fifo_global_name, 0600) != 0)
        {
            perror(NULL);
            err_printf("can't create FIFO_global\n");
            unlink(name);
            close(fd_global);
            close(fd_private);
            exit(EXIT_FAILURE);
        }
        fd_global = open(fifo_global_name, O_RDWR | O_WRONLY);
        if (fd_global < 0)
        {
            err_printf("can't open FIFO_global\n");
            unlink(name);
            close(fd_global);
            close(fd_private);
            exit(EXIT_FAILURE);
        }
    }

    pid_t receiver_pid = getpid();
    if (write(fd_global, &receiver_pid, sizeof(pid_t)) != sizeof(pid_t))
    {
        {
            err_printf("can't open FIFO\n");
            unlink(name);
            close(fd_global);
            close(fd_private);
            exit(EXIT_FAILURE);
        }
    }

    char buff[BUFF_SIZE];

    if (WaitForSender(fd_private))
    {
        err_printf("init failed\n");
        unlink(name);
        close(fd_private);
        close(fd_private);
        exit(EXIT_FAILURE);
    }
    // change fd flags to blocking node
    fcntl(fd_private, F_SETFL, O_RDONLY);

    ssize_t read_amount = 0; // reuse of variabel
    do
    {
        read_amount = read(fd_private, buff, BUFF_SIZE);
        if (read_amount < 0)
        {
            unlink(name);
            close(fd_private);
            close(fd_private);
            err_printf("error has occurred in reading");
            exit(EXIT_FAILURE);
        }
        if (read_amount == 0)
        {
            unlink(name);
            close(fd_private);
            close(fd_private);
            return EXIT_SUCCESS;
        }
        write(1, buff, read_amount);
    } while (read_amount > 0);

    unlink(name);
    close(fd_private);
    close(fd_private);
    return EXIT_FAILURE;
}

int FindFifo(char *name)
{
    // TODO
    // open fifo and read pid_t form pipe && and create name for private fifo
    int fd_global = open(fifo_global_name, O_RDONLY | O_NDELAY); // fd of FIFO
    if (fd_global < 0)
    {
        return EXIT_FAILURE;
    }
    pid_t receiver_pid = -1;
    if (read(fd_global, &receiver_pid, sizeof(pid_t)) != sizeof(pid_t))
    {
        close(fd_global);
        err_printf("no sender found");
        return EXIT_FAILURE;
    }
    close(fd_global);
    PrepareName(name, receiver_pid);
    return EXIT_SUCCESS;
}

/**
 * @brief Preparing name for fifo file
 * 
 * @param buff_out 
 */
void PrepareName(char *buff_out, pid_t pid)
{
    // prepare name for fifo
    strcpy(buff_out, dir_for_fifo);
    char buff_for_pid[ID_LENGTH];
    sprintf(buff_for_pid, "%s%d", fifo_name, pid);
    strcat(buff_out, buff_for_pid);
    strcat(buff_out, file_name_type);
}

/**
 * @brief Wait for opening receiver and q
 * 
 * @param fd fd for FIFO
 * @return int 
 */
int WaitForSender(int fd)
{
    // TODO add size of file to send to FIFO
    ssize_t write_bits = 0; // ssize_t == long int
    for (size_t i = 0; i < SEND_ATTEMPTS; i++)
    {
        char buff[BUFF_SIZE];
        write_bits = read(fd, buff, START_CODE_SIZE);
        if (write_bits > 0)
        {
            return EXIT_SUCCESS;
        }
        err_printf("read attempt failed. Try again in %d sec\n", WAIT_TIMEOUT);
        sleep(WAIT_TIMEOUT);
    }
    return EXIT_FAILURE;
}

int SendFile(const char *file_name)
{
    // TODO make this str operations safe
    char name[FIFO_NAME_SIZE];
    // prepare name for fifo
    //PrepareName(name);

    if (FindFifo(name))
    {
        err_printf("\tno sender found\n");
        exit(EXIT_FAILURE);
    }
    int fd = open(name, O_WRONLY | O_NDELAY); // fd of FIFO
    if (fd < 0)
    {
        err_printf("can't open FIFO or no fifo\n");
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    if (write(fd, start_code, START_CODE_SIZE) < 0)
    {
        err_printf("can't init connection\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    int file_for_read = open(file_name, O_RDONLY);
    if (file_for_read < 0)
    {
        err_printf("can't open file : %s\n", file_name);
        close(fd);
        exit(EXIT_FAILURE);
    }
    char buff[BUFF_SIZE];
    int read_amount = 0;
    int write_amount = 0;
    do
    {
        read_amount = read(file_for_read, buff, BUFF_SIZE);
        write_amount = write(fd, buff, read_amount);
        err_printf("writted : %d\n", write_amount);
        if (write_amount < 0)
        {
            err_printf("error in writing occurred\n");
            close(fd);
            close(file_for_read);
            exit(EXIT_FAILURE);
        }
    } while (read_amount > 0);
    close(file_for_read);

    close(fd);
    return EXIT_SUCCESS;
}
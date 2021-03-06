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
//#include </usr/include/linux/fcntl.h> // specific for linux for using F_SETPIPE_SZ
#define _DEFAULT_SOURCE

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

#define ID_LENGTH 22 // 5 for "/tmp/" , 11 for 4 bite unsigned int , 5 for ".fifo" , and 1 for "\0"

#define WAIT_TIMEOUT 3
#define SEND_ATTEMPTS 5

#define BUFF_SIZE 4096 // size of buffer used to send file with write/read

typedef unsigned int id_t;

#define START_CODE_SIZE 1
const char start_code[START_CODE_SIZE] = {'~'};

const char *mode_send = "-send";
const char *mode_receive = "-receive";

const char *dir_for_fifo = "/tmp/";
const char *file_name_type = ".fifo";
const char *fifo_name = "fifo_alek";

#define FIFO_NAME_SIZE 10

int ReceiveFile();
int WaitForSender(int fd);
int SendFile(const char *file_name);
void PrepareName(char *buff_out);
int FindFifo(char *name);

int main(int argc, char **argv)
{
    //TODO rewrite it with normal style of arg parsing
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
 * @brief find for fifo with format
 * 
 * @param name 
 * @return int 
 */
int FindFifo(char *name)
{
    strcpy(name, dir_for_fifo);
    DIR *d;
    struct dirent *dir;
    d = opendir("/tmp/");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strlen(dir->d_name) > FIFO_NAME_SIZE && dir->d_type == DT_FIFO)
            {
                bool flag = true;
                for (size_t i = 0; i < FIFO_NAME_SIZE - 1; i++)
                {
                    if (*(dir->d_name + i) != *(fifo_name + i))
                    {
                        flag = false;
                    }
                }
                if (flag)
                {
                    strcat(name,dir->d_name);
                    closedir(d);
                    return EXIT_SUCCESS;
                }
            }
        }
        closedir(d);
        return EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}

/**
 * @brief Make attempt to send file to receive by id
 * 
 * @param id string with id
 * @param file_name name of file to be sended
 * @return int 
 */
int SendFile(const char *file_name)
{
    // TODO make this str operations safe
    char name[ID_LENGTH];
    // prepare name for fifo
    //PrepareName(name);

    if (FindFifo(name))
    {
        err_printf("no fifo found\n");
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

    // rm file with fifo from drive
    if (unlink(name))
    {
        err_printf("can't delite FIFO\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // change open mode to bloking
    fcntl(fd, F_SETFL, O_WRONLY);

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
            if (*buff == *start_code)
            {
                return EXIT_SUCCESS;
            }
            err_printf("start code isn't correct");
            return EXIT_FAILURE;
        }
        err_printf("read attempt failed. Try again in %d sec\n", WAIT_TIMEOUT);
        sleep(WAIT_TIMEOUT);
    }
    return EXIT_FAILURE;
}

/**
 * @brief Preparing name for fifo file
 * 
 * @param buff_out 
 */
void PrepareName(char *buff_out)
{
    pid_t pid = getpid();
    // prepare name for fifo
    strcpy(buff_out, dir_for_fifo);
    char buff_for_pid[ID_LENGTH];
    sprintf(buff_for_pid, "%s%d", fifo_name, pid);
    strcat(buff_out, buff_for_pid);
    strcat(buff_out, file_name_type);
}

/**
 * @brief Init connection and read data from FIFO
 * 
 * @param id 
 * @return int 
 */
int ReceiveFile()
{
    // TODO make this str operations safe
    char name[ID_LENGTH];

    while(!FindFifo(name))
    {
        if(unlink(name))
        {
            err_printf("error in removing old fifo");
        }
    }


    PrepareName(name);


    int fd = open(name, O_RDONLY | O_NDELAY); // fd of FIFO

    if (fd < 0)
    {
        umask(0); // set mask to 0000
        if (mkfifo(name, 0600) != 0)
        {
            perror(NULL);
            err_printf("can't create FIFO\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
        fd = open(name, O_RDONLY | O_NDELAY);
        if (fd < 0)
        {
            err_printf("can't open FIFO\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    /* if(fcntl(fd,F_SETPIPE_SZ,BUFF_SIZE) != BUFF_SIZE)
    {
        err_printf("can't set FIFO size\n");
    } */

    char buff[BUFF_SIZE];

    if (WaitForSender(fd))
    {
        err_printf("init failed\n");
        unlink(name);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // change fd flags to blocking node
    fcntl(fd, F_SETFL, O_RDONLY);

    ssize_t read_amount = 0; // reuse of variabel
    do
    {
        read_amount = read(fd, buff, BUFF_SIZE);
        if (read_amount < 0)
        {
            close(fd);
            err_printf("error has occurred in reading");
            exit(EXIT_FAILURE);
        }
        if (read_amount == 0)
        {
            close(fd);
            return EXIT_SUCCESS;
        }
        write(1, buff, read_amount);
    } while (read_amount > 0);

    close(fd);
    return EXIT_FAILURE;
}

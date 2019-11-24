/**
 * @file queue_main.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief Task 2 for half of plus for cs 
 * @version 0.1
 * @date 2019-10-04
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>

#include "../Custom_libs/Lib_funcs.h"

// TODO remove i form func call
void child_function(size_t i);

int msg_id_1 = -1;
int msg_id_2 = -1;

struct mymsgbuf_index
{
    long mtype;
};

struct mymsgbuf_pid
{
    long mtype;
    int child_number;
};

#define MTYPE_FOR_CHILD_PID 1

int main(int argc, char **argv)
{
    // check input args
    if (argc != 2)
    {
        err_printf("Bad amount of args\n");
        return -1;
    }

    // check input number
    long N = ReadPosNumberFromArg(1, argv);

    if (N < 0)
    {
        err_printf("error in number format\n");
        return -1;
    }
    if (N < 1)
    {
        err_printf("number of child process must be above 0\n");
        return -1;
    }

    //crate message queue
    msg_id_1 = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600); // crate msg with uniq id and perms for write and read only for user
    if (msg_id_1 < 0)
    {
        perror("can't create message queue");
        msgctl(msg_id_1, IPC_RMID, NULL);
        return -1;
    }
    msg_id_2 = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600); // crate msg with uniq id and perms for write and read only for user
    if (msg_id_2 < 0)
    {
        perror("can't create message queue");
        msgctl(msg_id_2, IPC_RMID, NULL);
        msgctl(msg_id_1, IPC_RMID, NULL);
        return -1;
    }

    // create child precesses
    for (size_t i = 1; i <= N; i++) // from 1 because of msgrcv type behaviour
    {
        pid_t pid = fork();

        // for parent
        if (pid > 0)
        {
            //printf("Parent forked child with pid = %d\n", pid);
        }
        else if (pid != 0)
        {
            printf("#### error in fork happened ####\n");
            msgctl(msg_id_2, IPC_RMID, NULL);
            msgctl(msg_id_1, IPC_RMID, NULL);
            return -1;
        }

        // for child
        if (pid == 0)
        {
            child_function(i);
            return 0;
            //close process
        }
    }

    for (size_t i = 1; i <= N; i++)
    {
        struct mymsgbuf_index buf_with_index;
        buf_with_index.mtype = i;

        if (msgsnd(msg_id_1, &buf_with_index, 0, 0))
        {
            perror("parent can't send message with index to child");
            msgctl(msg_id_2, IPC_RMID, NULL);
            msgctl(msg_id_1, IPC_RMID, NULL);
            exit(-1);
        }

        struct mymsgbuf_pid buf_ready_ans;
        if (msgrcv(msg_id_2, &buf_ready_ans, sizeof(int), MTYPE_FOR_CHILD_PID, 0) == -1)
        {
            perror("parent can't receive message from child about printing\n");
            msgctl(msg_id_2, IPC_RMID, NULL);
            msgctl(msg_id_1, IPC_RMID, NULL);
            exit(-1);
        }
    }

    msgctl(msg_id_2, IPC_RMID, NULL);
    msgctl(msg_id_1, IPC_RMID, NULL);
    return 0;
}

void child_function(size_t i)
{
    pid_t local_pid = getpid();

    // wait for msg from parent to print i
    struct mymsgbuf_index buf_with_index;
    if (msgrcv(msg_id_1, &buf_with_index, 0, i, 0) == -1)
    {
        perror("child can't receive message from parent to print\n");
        exit(-1);
    }

    printf("\tI'm child %d and I has burned %ld in order\n", local_pid, i);

    struct mymsgbuf_pid buf_with_pid;
    buf_with_pid.mtype = MTYPE_FOR_CHILD_PID;
    buf_with_pid.child_number = i;
    if (msgsnd(msg_id_2, &buf_with_pid, sizeof(int), 0))
    {
        perror("parent can't send message with index to child");
        exit(-1);
    }
}

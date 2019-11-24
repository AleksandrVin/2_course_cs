/**
 * @file test_message_queue_iml.c
 * @author your name (you@domain.com)
 * @brief to test message queue implementation in system
 * @version 0.1
 * @date 2019-10-23
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>

int main()
{
    int id = msgget(IPC_PRIVATE, 0600 | IPC_CREAT | IPC_EXCL );
    if(id == -1)
    {
        perror("can't crate message queque\n");
        exit(-1);
    }
    printf("message queue created with id = %d", id);
    if(msgctl(id,IPC_RMID,NULL))
    {
        perror("can't delite message queue\n");
        exit(-1);
    }
    return 0;
}
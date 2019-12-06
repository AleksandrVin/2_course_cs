/**
 * @file file_transfer.c
 * @author Aleksandr Vinogradov (vinogradov.alek@gmail.com)
 * @brief File transfer between console in one OS with shared memory 
 * @version 0.1
 * @date 2019-12-01
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

#include <time.h> // for timespec

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

//extern semtimedop(int, struct sembuf *, size_t, const struct timespec *);

#include "../Custom_libs/Lib_funcs.h"

#define ARG_MODE 1
#define ARG_NAME 2

#define WAIT_TIMEOUT 3
#define WAIT_TIMEOUT_LONG 10

#define NAME_SIZE 1024

// shm ->> | write_amount size | shm_mem_size |
#define SHM_MEM_SIZE (4096 - sizeof(int))
#define WRITE_AMOUNT_SIZE sizeof(int)

#define SEM_NUM_AMOUNT 3

#define SEM_NUM_WR_MUTEX 0
#define SEM_NUM_EMPTY 1
#define SEM_NUM_FULL 2

const char mode_send[] = "-send";
const char mode_receive[] = "-receive";

const char fifo_start_global_name[] = "/tmp/global_start_process_task_3.fifo";
const char shm_mem_name[] = "/tmp/shm_mem_alek_3.shmmem";
const char sem_global_acces_to_shmmem_file[] = "/tmp/sem_global_access_to_shmmem_file.sem";

int ReceiveFile();
int SendFile(const char *file_name);

pid_t GetReceiverPId();
int WaitForSender(int fd);

bool clean_sem_shm(int shm_id, int sem_id);

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
    printf("\tWaiting for sender. Timeout is %d sec\n\n", WAIT_TIMEOUT_LONG);

    // create files for ftok
    int shm_key_file_fd = creat(shm_mem_name, S_IRWXU);
    if (shm_key_file_fd == -1)
    {
        perror("can't open or create file for shm key\n");
        exit(EXIT_FAILURE);
    }
    close(shm_key_file_fd);

    int sem_key_file_fd = creat(sem_global_acces_to_shmmem_file, S_IRWXU);
    if (shm_key_file_fd == -1)
    {
        perror("can't open or create file for sem key\n");
        exit(EXIT_FAILURE);
    }
    close(sem_key_file_fd);

    // init shm_mem
    pid_t receiver_pid = getpid();

    key_t key_shm = ftok(shm_mem_name, receiver_pid);
    if (key_shm == -1)
    {
        perror("unable to ftok key for smh\n");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(key_shm, SHM_MEM_SIZE + WRITE_AMOUNT_SIZE, IPC_CREAT | 0666);
    if (shm_id == -1)
    {
        perror("can't create shm_mem segmet\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    char *shm_adr = shmat(shm_id, NULL, SHM_RDONLY);
    if (shm_adr == (char *)-1)
    {
        perror("can't get addred for shm\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    // init sem
    key_t key_sem = ftok(sem_global_acces_to_shmmem_file, receiver_pid);
    if (key_sem == -1)
    {
        perror("unable to ftok key for sem\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    int sem_id = semget(key_sem, SEM_NUM_AMOUNT, IPC_CREAT | 0666);
    if (sem_id == -1)
    {
        perror("can't create sem array\n");
        clean_sem_shm(shm_id, sem_id);
        exit(EXIT_FAILURE);
    }

    // sender process waits for parent
    // global fifo for synchronization
    int fd_fifo_global = open(fifo_start_global_name, O_RDWR | O_NDELAY); // fd of FIFO

    if (fd_fifo_global < 0)
    {
        umask(0); // set mask to 0000
        if (mkfifo(fifo_start_global_name, 0600) != 0)
        {
            perror("can't create FIFO_global\n");
            clean_sem_shm(shm_id, sem_id);
            exit(EXIT_FAILURE);
        }
        fd_fifo_global = open(fifo_start_global_name, O_RDWR | O_NDELAY);
        if (fd_fifo_global < 0)
        {
            perror("can't open FIFO_global\n");
            clean_sem_shm(shm_id, sem_id);
            exit(EXIT_FAILURE);
        }
    }

    if (write(fd_fifo_global, &receiver_pid, sizeof(pid_t)) != sizeof(pid_t))
    {
        {
            perror("can't write init pid to FIFO\n");
            clean_sem_shm(shm_id, sem_id);
            exit(EXIT_FAILURE);
        }
    }

    if (WaitForSender(sem_id) == EXIT_FAILURE)
    {
        err_printf("waiting for sender error\n");
        clean_sem_shm(shm_id, sem_id);
        exit(EXIT_FAILURE);
    }

    close(fd_fifo_global);
    unlink(fifo_start_global_name);

    int amount_to_read = 0; // printed to stdout
    do
    {
        // P(full)
        struct sembuf sops;
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_FULL;
        sops.sem_op = -1;

        struct timespec timeout;
        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;

        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("wait for sender empty release timeout\n");
            clean_sem_shm(shm_id, sem_id);
            return EXIT_FAILURE;
        }
        // P(mutex)
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_WR_MUTEX;
        sops.sem_op = -1;

        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;
        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("wait for sender mutex realease timeout\n");
            clean_sem_shm(shm_id, sem_id);
            return EXIT_FAILURE;
        }

        amount_to_read = *((int *)shm_adr);
        if (amount_to_read == 0)
        {
            break;
        }
        else if (amount_to_read < 0)
        {
            err_printf("strange error in sender\n");
            clean_sem_shm(shm_id, sem_id);
            exit(EXIT_FAILURE);
        }

        int printed = write(1, (shm_adr + WRITE_AMOUNT_SIZE), amount_to_read);
        if (printed != amount_to_read)
        {
            perror("error in printing to stdout\n");
            clean_sem_shm(shm_id, sem_id);
            exit(EXIT_FAILURE);
        }

        // V(mutex)
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_WR_MUTEX;
        sops.sem_op = 1;

        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;
        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("wait for sender mutex realease timeout\n");
            return EXIT_FAILURE;
        }

        // V(empty)
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_EMPTY;
        sops.sem_op = 1;

        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;
        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("wait for sender mutex realease timeout\n");
            return EXIT_FAILURE;
        }

    } while (amount_to_read > 0);

    return clean_sem_shm(shm_id, sem_id);
}

/**
 * @brief Wait for starting transfer
 * 
 * @param sem_id semaphore id
 * @return int 
 */
int WaitForSender(int sem_id)
{
    //set sem to the init values
    struct sembuf sops;
    // V(mutex)
    sops.sem_flg = 0;
    sops.sem_num = SEM_NUM_WR_MUTEX;
    sops.sem_op = 1;

    if (semop(sem_id, &sops, 1) != 0)
    {
        perror("can't set init value to start sending\n");
        return EXIT_FAILURE;
    }

    // V(empty)
    sops.sem_flg = 0;
    sops.sem_num = SEM_NUM_EMPTY;
    sops.sem_op = 1;

    if (semop(sem_id, &sops, 1) != 0)
    {
        perror("can't set init value to start sending\n");
        return EXIT_FAILURE;
    }

    // wait to sem be 0 by sender
    sops.sem_flg = 0;
    sops.sem_num = SEM_NUM_WR_MUTEX;
    sops.sem_op = 0;

    struct timespec timeout;
    timeout.tv_sec = WAIT_TIMEOUT_LONG;
    timeout.tv_nsec = WAIT_TIMEOUT_LONG;
    if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
    {
        perror("no sender detected\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

bool clean_sem_shm(int shm_id, int sem_id)
{
    bool status = true;
    if (shm_id)
        status &= shmctl(shm_id, IPC_RMID, NULL);
    if (sem_id)
        status &= semctl(sem_id, 0, IPC_RMID, 0);
    return status;
}

int SendFile(const char *file_name)
{
    // open file to be sended
    int file_for_read = open(file_name, O_RDONLY);
    if (file_for_read < 0)
    {
        err_printf("can't open file : %s\n", file_name);
        close(file_for_read);
        exit(EXIT_FAILURE);
    }

    // get pid from receiver
    pid_t receiver_pid = GetReceiverPId();
    if (receiver_pid < 0)
    {
        perror("can't get receiver pid\n");
        return EXIT_FAILURE;
    }

    // init shm
    key_t key_shm = ftok(shm_mem_name, receiver_pid);
    if (key_shm == -1)
    {
        perror("unable to ftok key for smh\n");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(key_shm, SHM_MEM_SIZE + WRITE_AMOUNT_SIZE, IPC_CREAT | 0666);
    if (shm_id == -1)
    {
        perror("can't create shm_mem segmet\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    char *shm_adr = shmat(shm_id, NULL, 0);
    if (shm_adr == (char *)-1)
    {
        perror("can't get addred for shm\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    // init sem
    key_t key_sem = ftok(sem_global_acces_to_shmmem_file, receiver_pid);
    if (key_sem == -1)
    {
        perror("unable to ftok key for sem\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    int sem_id = semget(key_sem, SEM_NUM_AMOUNT, IPC_CREAT | 0666);
    if (sem_id == -1)
    {
        perror("can't create sem array\n");
        clean_sem_shm(shm_id, sem_id);
        exit(EXIT_FAILURE);
    }

    struct sembuf sops;
    struct timespec timeout;

    printf("\t Sending \n\n");

    int read_amount = 0;
    do
    {
        // P(empty)
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_EMPTY;
        sops.sem_op = -1;

        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;
        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("send timeout error 1\n");
            clean_sem_shm(shm_id, sem_id);
            return EXIT_FAILURE;
        }

        // P(mutex)
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_WR_MUTEX;
        sops.sem_op = -1;

        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;
        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("send timeout error 2\n");
            clean_sem_shm(shm_id, sem_id);
            return EXIT_FAILURE;
        }

        read_amount = read(file_for_read, (shm_adr + WRITE_AMOUNT_SIZE), SHM_MEM_SIZE);
        *((int *)shm_adr) = read_amount;

        if (read_amount < 0)
        {
            perror("error in writing occurred\n");
            clean_sem_shm(shm_id, sem_id);
            close(file_for_read);
            exit(EXIT_FAILURE);
        }

        // V(mutex)
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_WR_MUTEX;
        sops.sem_op = 1;

        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;
        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("send timeout error 1\n");
            clean_sem_shm(shm_id, sem_id);
            return EXIT_FAILURE;
        }

        // V(full)
        sops.sem_flg = 0;
        sops.sem_num = SEM_NUM_FULL;
        sops.sem_op = 1;

        timeout.tv_sec = WAIT_TIMEOUT;
        timeout.tv_nsec = WAIT_TIMEOUT;
        if (semtimedop(sem_id, &sops, 1, &timeout) != 0)
        {
            perror("send timeout error 2\n");
            clean_sem_shm(shm_id, sem_id);
            return EXIT_FAILURE;
        }

    } while (read_amount > 0);
    close(file_for_read);

    printf("\t Done \n");
    return EXIT_SUCCESS;
}

pid_t GetReceiverPId()
{
    // open fifo and read pid_t form pipe
    int fd_global = open(fifo_start_global_name, O_RDONLY | O_NDELAY); // fd of FIFO
    if (fd_global < 0)
    {
        perror("can't open fifo to receive pid from receiver\n");
        return -1;
    }

    pid_t receiver_pid = -1;
    if (read(fd_global, &receiver_pid, sizeof(pid_t)) != sizeof(pid_t))
    {
        perror("no sender found\n");
        close(fd_global);
        return -1;
    }

    close(fd_global);

    if (receiver_pid < 0)
    {
        perror("bad pid from receiver\n");
        return -1;
    }
    return receiver_pid;
}
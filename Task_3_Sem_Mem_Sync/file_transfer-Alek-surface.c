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

#include "../Custom_libs/Lib_funcs.h"

#define ARG_MODE 1
#define ARG_NAME 2

#define WAIT_TIMEOUT 3
#define WAIT_TIMEOUT_LONG 10

#define NAME_SIZE 1024

// shm ->> | write_amount size | shm_mem_size |
#define SHM_MEM_SIZE (4096 - sizeof(int))
#define WRITE_AMOUNT_SIZE sizeof(int)

#define SEM_NUM_AMOUNT 6

#define SEM_NUM_WR_MUTEX 0
#define SEM_NUM_EMPTY 1
#define SEM_NUM_FULL 2
#define SEM_NUM_S_DEATH 3
#define SEM_NUM_R_DEATH 4
#define SEM_NUM_INIT 5

const char mode_send[] = "-send";
const char mode_receive[] = "-receive";

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

    key_t key_shm = ftok(shm_mem_name, 0);
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
    key_t key_sem = ftok(sem_global_acces_to_shmmem_file, 0);
    if (key_sem == -1)
    {
        perror("unable to ftok key for sem\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    int sem_id = semget(key_sem, SEM_NUM_AMOUNT, IPC_CREAT | 0666);
    if (sem_id == -1)
    {
        perror("can't create sem array or sem already exist\n");
        clean_sem_shm(shm_id, sem_id);
        exit(EXIT_FAILURE);
    }

    // V(death)
    struct sembuf sops_3[3];
    sops_3[0].sem_flg = IPC_NOWAIT;
    sops_3[0].sem_num = SEM_NUM_R_DEATH;
    sops_3[0].sem_op = 0;

    sops_3[1].sem_flg = IPC_NOWAIT;
    sops_3[1].sem_num = SEM_NUM_INIT;
    sops_3[1].sem_op = 0;

    sops_3[2].sem_flg = SEM_UNDO;
    sops_3[2].sem_num = SEM_NUM_R_DEATH;
    sops_3[2].sem_op = 1;

    if (semop(sem_id, sops_3, 3) != 0)
    {
        perror("can't set receiver inits sem receiver already exists\n");
        exit(EXIT_FAILURE);
    }

    if (WaitForSender(sem_id) == EXIT_FAILURE)
    {
        err_printf("waiting for sender error\n");
        clean_sem_shm(shm_id, sem_id);
        exit(EXIT_FAILURE);
    }

    bool interation = false;
    int amount_to_read = 0; // printed to stdout
    do
    {
        struct sembuf sops_5[5];
        if(interation)
        {
// P(S)
        sops_5[0].sem_flg = IPC_NOWAIT;
        sops_5[0].sem_num = SEM_NUM_S_DEATH;
        sops_5[0].sem_op = -1;
        // V(S)
        sops_5[1].sem_flg = 0;
        sops_5[1].sem_num = SEM_NUM_S_DEATH;
        sops_5[1].sem_op = 1;

        // P(full)
        sops_5[2].sem_flg = SEM_UNDO;
        sops_5[2].sem_num = SEM_NUM_FULL;
        sops_5[2].sem_op = -1;

        // P(full)
        sops_5[2].sem_flg = SEM_UNDO;
        sops_5[2].sem_num = SEM_NUM_FULL;
        sops_5[2].sem_op = -1;
        // P(full)
        sops_5[2].sem_flg = SEM_UNDO;
        sops_5[2].sem_num = SEM_NUM_FULL;
        sops_5[2].sem_op = -1;

        if (semop(sem_id, sops_3, 3) != 0)
        {
            err_printf("sender error accrued\n");
            clean_sem_shm(shm_id, sem_id);
            exit(EXIT_FAILURE);
        }
        }
        // P(S)
        sops_3[0].sem_flg = IPC_NOWAIT;
        sops_3[0].sem_num = SEM_NUM_S_DEATH;
        sops_3[0].sem_op = -1;
        // V(S)
        sops_3[1].sem_flg = 0;
        sops_3[1].sem_num = SEM_NUM_S_DEATH;
        sops_3[1].sem_op = 1;

        // P(full)
        sops_3[2].sem_flg = SEM_UNDO;
        sops_3[2].sem_num = SEM_NUM_FULL;
        sops_3[2].sem_op = -1;

        if (semop(sem_id, sops_3, 3) != 0)
        {
            err_printf("sender error accrued\n");
            clean_sem_shm(shm_id, sem_id);
            exit(EXIT_FAILURE);
        }

        // P(mutex)
        sops_3[2].sem_flg = SEM_UNDO;
        sops_3[2].sem_num = SEM_NUM_WR_MUTEX;
        sops_3[2].sem_op = -1;

        if (semop(sem_id, sops_3, 3) != 0)
        {
            perror("wait for sender empty release error\n");
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
        sops_3[2].sem_flg = SEM_UNDO;
        sops_3[2].sem_num = SEM_NUM_WR_MUTEX;
        sops_3[2].sem_op = 1;

        if (semop(sem_id, sops_3, 3) != 0)
        {
            perror("wait for sender mutex realease timeout\n");
            return EXIT_FAILURE;
        }
        

        // P(S)
        sops_5[0].sem_flg = IPC_NOWAIT;
        sops_5[0].sem_num = SEM_NUM_S_DEATH;
        sops_5[0].sem_op = -1;
        // V(S)
        sops_5[1].sem_flg = 0;
        sops_5[1].sem_num = SEM_NUM_S_DEATH;
        sops_5[1].sem_op = 1;

        // V(empty)
        sops_5[2].sem_flg = SEM_UNDO;
        sops_5[2].sem_num = SEM_NUM_EMPTY;
        sops_5[2].sem_op = 1;

        // V(full)
        sops_5[3].sem_flg = SEM_UNDO;
        sops_5[3].sem_num = SEM_NUM_FULL;
        sops_5[3].sem_op = 1;

        // P(full)
        sops_5[4].sem_flg = 0;
        sops_5[4].sem_num = SEM_NUM_FULL;
        sops_5[4].sem_op = -1;

        if (semop(sem_id, sops_5, 5) != 0)
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
    struct sembuf sops_4[4];
    // P(I)
    sops_4[0].sem_flg = 0;
    sops_4[0].sem_num = SEM_NUM_INIT;
    sops_4[0].sem_op = -1;

    // V(I)
    sops_4[1].sem_flg = SEM_UNDO;
    sops_4[1].sem_num = SEM_NUM_INIT;
    sops_4[1].sem_op = 1;

    sops_4[2].sem_flg = SEM_UNDO;
    sops_4[2].sem_num = SEM_NUM_WR_MUTEX;
    sops_4[2].sem_op = 1;

    sops_4[3].sem_flg = SEM_UNDO;
    sops_4[3].sem_num = SEM_NUM_EMPTY;
    sops_4[3].sem_op = 1;

    struct timespec timeout;
    timeout.tv_sec = WAIT_TIMEOUT_LONG;
    timeout.tv_nsec = WAIT_TIMEOUT_LONG;
    if (semtimedop(sem_id, sops_4, 4, &timeout) != 0)
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

    // init shm
    key_t key_shm = ftok(shm_mem_name, 0);
    if (key_shm == -1)
    {
        perror("unable to ftok key for smh\n");
        exit(EXIT_FAILURE);
    }

    int shm_id = shmget(key_shm, SHM_MEM_SIZE + WRITE_AMOUNT_SIZE, 0666);
    if (shm_id == -1)
    {
        perror("can't shm_mem segmet no receiver\n");
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
    key_t key_sem = ftok(sem_global_acces_to_shmmem_file, 0);
    if (key_sem == -1)
    {
        perror("unable to ftok key for sem\n");
        clean_sem_shm(shm_id, 0);
        exit(EXIT_FAILURE);
    }

    int sem_id = semget(key_sem, SEM_NUM_AMOUNT, 0666);
    if (sem_id == -1)
    {
        perror("can't create sem array no receiver\n");
        clean_sem_shm(shm_id, sem_id);
        exit(EXIT_FAILURE);
    }

    struct sembuf sops_6[6];

    // Z(I)
    sops_6[0].sem_flg = IPC_NOWAIT;
    sops_6[0].sem_num = SEM_NUM_INIT;
    sops_6[0].sem_op = 0;
    // Z(R)
    sops_6[1].sem_flg = IPC_NOWAIT;
    sops_6[1].sem_num = SEM_NUM_S_DEATH;
    sops_6[1].sem_op = 0;
    // P(R)
    sops_6[2].sem_flg = IPC_NOWAIT;
    sops_6[2].sem_num = SEM_NUM_R_DEATH;
    sops_6[2].sem_op = -1;
    // V(R)
    sops_6[3].sem_flg = 0;
    sops_6[3].sem_num = SEM_NUM_R_DEATH;
    sops_6[3].sem_op = 1;
    // V(I)
    sops_6[4].sem_flg = SEM_UNDO;
    sops_6[4].sem_num = SEM_NUM_INIT;
    sops_6[4].sem_op = 1;
    // V(S)
    sops_6[5].sem_flg = SEM_UNDO;
    sops_6[5].sem_num = SEM_NUM_S_DEATH;
    sops_6[5].sem_op = 1;

    if (semop(sem_id, sops_6, 6) != 0)
    {
        perror("sender can't init\n");
        exit(EXIT_FAILURE);
    }

    printf("\t Sending \n\n");

    bool iteration = false;
    int read_amount = 0;
    do
    {

        struct sembuf sops_3[3];
        struct sembuf sops_5[5];

        if (iteration)
        {
            // P(R)
            sops_5[0].sem_flg = IPC_NOWAIT;
            sops_5[0].sem_num = SEM_NUM_R_DEATH;
            sops_5[0].sem_op = -1;

            // V(R)
            sops_5[1].sem_flg = 0;
            sops_5[1].sem_num = SEM_NUM_R_DEATH;
            sops_5[1].sem_op = 1;

            // P(empty)
            sops_5[2].sem_flg = SEM_UNDO;
            sops_5[2].sem_num = SEM_NUM_EMPTY;
            sops_5[2].sem_op = -1;

            // V(full)
            sops_5[3].sem_flg = 0;
            sops_5[3].sem_num = SEM_NUM_EMPTY;
            sops_5[3].sem_op = 1;

            // P(full)
            sops_5[4].sem_flg = SEM_UNDO;
            sops_5[4].sem_num = SEM_NUM_EMPTY;
            sops_5[4].sem_op = -1;

            if (semop(sem_id, sops_5, 5) != 0)
            {
                perror("send error 1\n");
                clean_sem_shm(shm_id, sem_id);
                return EXIT_FAILURE;
            }
        }
        else
        {
            // P(R)
            sops_3[0].sem_flg = IPC_NOWAIT;
            sops_3[0].sem_num = SEM_NUM_R_DEATH;
            sops_3[0].sem_op = -1;

            // V(R)
            sops_3[1].sem_flg = 0;
            sops_3[1].sem_num = SEM_NUM_R_DEATH;
            sops_3[1].sem_op = 1;

            // P(empty)
            sops_3[2].sem_flg = SEM_UNDO;
            sops_3[2].sem_num = SEM_NUM_EMPTY;
            sops_3[2].sem_op = -1;

            if (semop(sem_id, sops_3, 3) != 0)
            {
                perror("send error 1\n");
                clean_sem_shm(shm_id, sem_id);
                return EXIT_FAILURE;
            }
        }

        iteration = true;

        // P(mutex)
        sops_3[2].sem_flg = SEM_UNDO;
        sops_3[2].sem_num = SEM_NUM_WR_MUTEX;
        sops_3[2].sem_op = -1;

        if (semop(sem_id, sops_3, 3) != 0)
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
        sops_3[2].sem_flg = SEM_UNDO;
        sops_3[2].sem_num = SEM_NUM_WR_MUTEX;
        sops_3[2].sem_op = 1;

        if (semop(sem_id, sops_3, 3) != 0)
        {
            perror("send timeout error 1\n");
            clean_sem_shm(shm_id, sem_id);
            return EXIT_FAILURE;
        }

        // P(R)
        sops_5[0].sem_flg = IPC_NOWAIT;
        sops_5[0].sem_num = SEM_NUM_R_DEATH;
        sops_5[0].sem_op = -1;

        // V(R)
        sops_5[1].sem_flg = 0;
        sops_5[1].sem_num = SEM_NUM_R_DEATH;
        sops_5[1].sem_op = 1;

        // V(full)
        sops_5[2].sem_flg = SEM_UNDO;
        sops_5[2].sem_num = SEM_NUM_FULL;
        sops_5[2].sem_op = 1;

        // V(empty)
        sops_5[3].sem_flg = SEM_UNDO;
        sops_5[3].sem_num = SEM_NUM_EMPTY;
        sops_5[3].sem_op = 1;

        // P(empty)
        sops_5[4].sem_flg = 0;
        sops_5[4].sem_num = SEM_NUM_EMPTY;
        sops_5[4].sem_op = 1;

        if (semop(sem_id, sops_5, 5) != 0)
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

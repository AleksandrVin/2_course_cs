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
#include <sys/sem.h>

#include "../Custom_libs/Lib_funcs.h"
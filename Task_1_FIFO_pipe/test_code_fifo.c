#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// for open call ( man 2 open )
#include <fcntl.h>

// for stat call ( man 2 stat )
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../Custom_libs/Lib_funcs.h"\

#define FIFO "/tmp/fifo_abc.fifo"

int main()
{
    umask(0);
    if(mkfifo(FIFO, 0666) == -1)
    {
        perror("fifo");
        unlink(FIFO);
        return EXIT_FAILURE;
    }
    int fd = open(FIFO, O_RDWR);
    write(fd, "aaa", 3);
    //close(fd);
    //fd= open("fifo", O_RDONLY, O_NONBLOCK);
    char buf[3];
    int amount =  read(fd, buf, 3);
    if(amount <= 0){
        perror("read");
        err_printf(" amount is %d",amount);
    }
    write(1 , buf, amount);
    unlink(FIFO);
    return EXIT_SUCCESS;
}
P = main
CFLAGS = -g -Wall -std=c11 -D_POSIX_C_SOURCE
LDFLAGS = -lpthread  -lm

CC = gcc

all: $(P)


$(P): $(P).c
	$(CC) $(CFLAGS) $(DEFINES) $^ -o $@.elf64 $(LDFLAGS)


# use this for trash_threads
# make DEFINES="-DTRASH_ALLOWED"
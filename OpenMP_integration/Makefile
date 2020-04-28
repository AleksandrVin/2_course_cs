P = main
CFLAGS = -g -Wall -std=c11
LDFLAGS = -fopenmp -lpthread -lgsl -lgslcblas -lm

CC = gcc

all: $(P)


$(P): $(P).c
	$(CC) $(CFLAGS) $(DEFINES) $^ -o $@.elf64 $(LDFLAGS)


# use this for trash_threads
# make DEFINES="-DTRASH_ALLOWED"
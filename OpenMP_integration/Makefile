P = main
CFLAGS = -g -Wall -std=c11
LDFLAGS = -fopenmp -lpthread -lgsl -lgslcblas -lm

CC = gcc

all: $(P)

$(P): $(P).c
	$(CC) $(CFLAGS) $^ -o $@.elf64 $(LDFLAGS)

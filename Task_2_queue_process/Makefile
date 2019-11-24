CC=gcc
CFLAGS=-g -Wall -std=c11 -O0 -lm -m64

P=queue_main

all: $(P)

.c:
	$(CC) $(CFLAGS) $< -o $@.elf64


run: $(P)
	./$<.elf64 10

	

clean:
	$(RM) *.elf64

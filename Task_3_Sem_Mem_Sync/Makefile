CC=gcc
CFLAGS=-g -Wall -std=c11 -O0 -lm -m64

P=file_transfer

all: $(P)

.c:
	$(CC) $(CFLAGS) $< -o $@.elf64


run_r: $(P)
	./$<.elf64 -receive 

run_s: $(P)
	./$<.elf64 -send test.txt
	

clean:
	$(RM) *.elf64

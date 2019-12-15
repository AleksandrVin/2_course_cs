CC=gcc
CFLAGS=-g -Wall -std=c11 -O0 -lm -m64

P=file_transfer

P2=file_comparator

P3=test_code_fifo

all: $(P) $(P2) $(P3)

.c:
	$(CC) $(CFLAGS) $< -o $@.elf64


run_r: $(P)
	./$<.elf64 -receive 

run_s: $(P)
	./$<.elf64 -send file_transfer_old.c

run_comp: $(P2)
	./$<.elf64 mkfifo.txt mkfifo.txt

run_fifo: $(P3)
	./$<.elf64 

clean:
	$(RM) *.elf64

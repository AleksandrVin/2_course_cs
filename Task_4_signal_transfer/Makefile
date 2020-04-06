CC=gcc
CFLAGS=-g -Wall -std=c11 -O0 -lm -m64

P=file_transfer

all: $(P)

.c:
	$(CC) $(CFLAGS) $< -o $@.elf64

run: $(P)
	./$<.elf64 test.txt
	

clean:
	$(RM) *.elf64

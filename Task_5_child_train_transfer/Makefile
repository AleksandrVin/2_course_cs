CC=gcc
CFLAGS=-g -Wall -std=c11 -O0 -m64
LDLIBS=-lm

P=file_transfer

all: $(P)

.c:
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@.elf64 $(LDLIBS)

run: $(P)
	./$<.elf64 2 test.txt
	

clean:
	$(RM) *.elf64

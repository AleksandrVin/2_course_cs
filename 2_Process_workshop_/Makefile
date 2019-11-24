CC=gcc
CFLAGS=-g -Wall -std=gnu11 -O0 -lm -m64

P_1=1_fork
P_2=2_exec
P_3=3_thread



PROGRAMS= $(P_1) $(P_2) $(P_3)

all: $(PROGRAMS)

$(P_1): $(P_1).c
	$(CC) $(CFLAGS) $< -o $@.elf64

$(P_2): $(P_2).c
	$(CC) $(CFLAGS) $< -o $@.elf64

$(P_3): $(P_3).c
	$(CC) $(CFLAGS) -pthread $< -o $@.elf64


run_$(P_1): $(P_1)
	./$<.elf64 100

run_$(P_2): $(P_2)
	./$<.elf64 ls -l -a -s -h

run_$(P_3): $(P_3)
	./$<.elf64 100000000000

clean: 
	$(RM) *elf64
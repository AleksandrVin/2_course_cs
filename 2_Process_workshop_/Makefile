CC=gcc
CFLAGS=-g -Wall -std=c11

all: 1_fork

1_fork: 1_fork.c
	$(CC) $(CFLAGS) $< -o $@

2_exex: 2_exex.c
	$(CC) $(CFLAGS) $< -o $@

3_thread: 3_thread.c
	$(CC) $(CFLAGS) -pthread $< -o $@	

run_2: 2_exec
	./$< ls -l -a -s -h

run: 3_thread
	./$< 130	


CC=gcc
CFLAGS=-g -Wall -std=c11


1_OpenFile_and_print: 1_OpenFile_and_print.c
	$(CC) $(CFLAGS) $< -o $@


run: 1_OpenFile_and_print
	./$< Lib_funcs.h


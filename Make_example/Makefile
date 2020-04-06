CFLAGS=-g -O3 -MD

all: a.out

a.out: 1.o 2.o
	$(CC) -o $@ $^

-include *.d

.PHONY: all


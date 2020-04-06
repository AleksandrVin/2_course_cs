P = List
CFLAGS=-g -Wall -std=c11 -O0 -m64 -pedantic

CFLAGS_GCOV = -fprofile-arcs -ftest-coverage
CFLAGS_DEBUG = -Dmalloc_debug

all: test

test: test_cases.c list.c
	$(CC) $(CFLAGS)  $(CFLAGS_DEBUG) -o $@.elf64 $^

add_gcov:
CFLAGS += $(CFLAGS_GCOV) 

run_gcov: add_gcov test
	./test.elf64 && gcov list.c

run_valgrind: test
	valgrind --leak-check=full ./test.elf64	

run_test: test
	time ./test.elf64

clean:
	rm -v *.gcda *.gcov *.gcno *.elf64
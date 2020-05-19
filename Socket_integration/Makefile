P1 = client
P2 = server
P3 = main
CFLAGS = -g -Wall -std=c++11 -D_POSIX_C_SOURCE
LDFLAGS = -fopenmp -lpthread -lm

CC = g++

all: $(P1) $(P2)


$(P1): $(P1).cpp
	$(CC) $(CFLAGS) $(DEFINES) $^ -o $@.elf64 $(LDFLAGS)

$(P2): $(P2).cpp
	$(CC) $(CFLAGS) $(DEFINES) $^ -o $@.elf64 $(LDFLAGS)

$(P3): $(P3).c
	$(CC) $(CFLAGS) $(DEFINES) $^ -o $@.elf64 $(LDFLAGS)

clean:
	rm *.elf64


# use this for trash_threads
# make DEFINES="-DTRASH_ALLOWED"
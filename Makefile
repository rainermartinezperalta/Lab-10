CC=gcc
CFLAGS=-Wall -g -pthread -std=c99
TARGETS=walker

all: $(TARGETS)

walker: walker.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o *~ $(TARGETS) a.out
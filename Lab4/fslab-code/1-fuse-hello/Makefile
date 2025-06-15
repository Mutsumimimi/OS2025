CFLAGS = -Wall -std=gnu11 -Wno-unused-variable
LDFLAGS =
LDLIBS = -lfuse3

CC=gcc

.PHONY: clean all

all: hello

hello: hello.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

hello.o: hello.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f hello *.o



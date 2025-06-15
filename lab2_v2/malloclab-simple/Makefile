#
# Students' Makefile for the Malloc Lab
#

CC = gcc 
CFLAGS = -g -fPIC -O2 -Wall

all: libmem.so libmem.a example

libmem.so: mm.o
	$(CC) $(CFLAGS) -shared -o libmem.so mm.o

libmem.a: mm.o
	ar rcs libmem.a mm.o

example: libmem.a example.c
	$(CC) $(CFLAGS) -o example -static example.c -L. -lmem

mm.o: mm.c
	$(CC) $(CFLAGS) -c mm.c -o mm.o

clean:
	rm -f *~ *.o libmem.so libmem.a example

.PHONY: all clean



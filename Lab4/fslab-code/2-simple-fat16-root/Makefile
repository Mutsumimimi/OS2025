CFLAGS = -Wall -std=gnu11 -Wno-unused-variable
LDFLAGS =
LDLIBS = -lfuse3

CC=gcc

.PHONY: clean debug static

all: simple_fat16

debug: CFLAGS += -g
debug: simple_fat16

static: CFLAGS += -static
static: simple_fat16

simple_fat16: simple_fat16.o fat16_fixed.o disk_simulator.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

fat16_fixed.o: fat16_fixed.c fat16.h
	$(CC) $(CFLAGS) -c -o $@ $<

simple_fat16.o: simple_fat16.c fat16.h fat16_utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

disk_simulator.o: disk_simulator.c disk_simulator.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f simple_fat16 *.o



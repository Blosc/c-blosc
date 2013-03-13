CC=gcc
CFLAGS=-O3 -g -msse2 -Wall
LDFLAGS=-lpthread
BLOSC_LIB= $(wildcard ../blosc/*.c)
SOURCES=bench.c $(BLOSC_LIB)
EXECUTABLE=bench

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

clean:
	rm -rf $(EXECUTABLE)

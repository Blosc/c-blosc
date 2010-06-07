CC=gcc
CFLAGS=-O3 -g -msse2 -Wall
LDFLAGS=-lpthread
SOURCES=bench.c ../src/blosc.c ../src/blosclz.c ../src/shuffle.c
EXECUTABLE=bench

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $@

clean:
	rm -rf $(EXECUTABLE)

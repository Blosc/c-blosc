CC=gcc
CFLAGS=-O3 -msse2 -Wall
LDFLAGS=-lpthread
SOURCES=test_api.c ../src/blosc.c ../src/blosclz.c ../src/shuffle.c
EXECUTABLE=test_api

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $@

clean:
	rm -rf test_api

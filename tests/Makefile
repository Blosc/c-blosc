CC=gcc
CFLAGS=-O3 -msse2 -Wall
LDFLAGS=-lpthread
BLOSC_LIB= ../src/blosc.c ../src/blosclz.c ../src/shuffle.c

# The list of executables
# Generated PNG (intermediate) files
SOURCES := $(wildcard *.c)
EXECUTABLES := $(patsubst %.c, %.exe, $(SOURCES))

.PHONY: all
all: test

test: $(EXECUTABLES)

%.exe: %.c $(BLOSC_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) "$<" $(BLOSC_LIB) -o "$@"

clean:
	rm -rf $(EXECUTABLES)

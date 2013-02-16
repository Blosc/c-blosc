CC=gcc
CFLAGS=-O3 -msse2 -Wall -pthread
LDFLAGS=-pthread
BLOSC_LIB= $(wildcard ../blosc/*.c)

# The list of executables
# Generated PNG (intermediate) files
SOURCES := $(wildcard *.c)
EXECUTABLES := $(patsubst %.c, %.exe, $(SOURCES))

.PHONY: all
all: $(EXECUTABLES)

test: $(EXECUTABLES)
	sh test_all.sh

%.exe: %.c $(BLOSC_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) "$<" $(BLOSC_LIB) -o "$@"

clean:
	rm -rf $(EXECUTABLES)

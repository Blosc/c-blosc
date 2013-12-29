CC=gcc
LZ4_INC =  $(wildcard ../lz4*)
CFLAGS=-O3 -g -msse2 -Wall -I$(LZ4_INC)
#SNAPPY_INC =  $(wildcard ../snappy*)
#CFLAGS=-O3 -g -msse2 -Wall -I$(LZ4_INC) -I$(SNAPPY_INC)
LDFLAGS=-lpthread
BLOSC_LIB= $(wildcard ../blosc/*.c)
BLOSC_LIB += $(wildcard ../lz4*/*.c)
#BLOSC_LIB += $(wildcard ../snappy*/*.cc)
SOURCES=bench.c $(BLOSC_LIB)
EXECUTABLE=bench

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

clean:
	rm -rf $(EXECUTABLE)

CC = clang++  # or g++ or just gcc if not compiling Snappy (C++ code)
CFLAGS = -O3 -g -msse2 -Wall
LDFLAGS = -lpthread
COMPLIBS_SRC = $(wildcard ../blosc/*.c)
EXECUTABLE = bench

# Support for internal LZ4
LZ4_DIR = ../internal-complibs/lz4-r109
CFLAGS += -DHAVE_LZ4 -I$(LZ4_DIR)
COMPLIBS_SRC += $(wildcard $(LZ4_DIR)/*.c)

# Support for external LZ4
#LDFLAGS += -DHAVE_LZ4 -llz4

# Support for internal Snappy
#SNAPPY_DIR = ../internal-complibs/snappy-1.1.1
#CFLAGS += -I$(SNAPPY_DIR)
#COMPLIBS_SRC += $(wildcard $(SNAPPY_DIR)/*.cc)

# Support for external Snappy
LDFLAGS += -DHAVE_SNAPPY -lsnappy

# Support for external Zlib
LDFLAGS += -DHAVE_ZLIB -lz

SOURCES = bench.c $(COMPLIBS_SRC)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

clean:
	rm -rf $(EXECUTABLE)

# Makefile for Aukadicty static libraries
# AmigaOS 3.1 compatible

CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -std=c99 -O2 -I./include -I./os-include -I./cjson -I./aukstreamcache/include
ARFLAGS = rcs

# Library names
LIB = lib/libaukadicty.a
LIB_STREAM = lib/libaukstreamcache.a

# Aukadicty source files
SRC = src/aukobject.c \
      src/aukarray.c \
      src/aukstring.c \
      src/aukfixed.c \
      src/aukproject.c \
      src/auktrack.c \
      src/auksound.c \
      src/auksoundfile.c \
      src/aukjson.c \
      src/auktyperegistry.c \
      cjson/cJSON.c

# AukStreamCache source files
SRC_STREAM = aukstreamcache/src/aukstream.c \
             aukstreamcache/src/aukstreampool.c \
             aukstreamcache/src/aukstreamwave.c \
             aukstreamcache/src/aukstream8svx.c \
             aukstreamcache/src/aukstreamconvert.c \
             aukstreamcache/src/aukstreamloader.c

# Object files
OBJ = $(SRC:.c=.o)
OBJ_STREAM = $(SRC_STREAM:.c=.o)

# Default target - build both libraries
all: $(LIB) $(LIB_STREAM)

# Build aukadicty library
$(LIB): $(OBJ)
	@mkdir -p lib
	$(AR) $(ARFLAGS) $@ $^
	@echo "Library built: $(LIB)"

# Build aukstreamcache library
$(LIB_STREAM): $(OBJ_STREAM)
	@mkdir -p lib
	$(AR) $(ARFLAGS) $@ $^
	@echo "Library built: $(LIB_STREAM)"

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJ) $(OBJ_STREAM) $(LIB) $(LIB_STREAM)
	@echo "Cleaned build artifacts"

# Rebuild everything
rebuild: clean all

# Build examples (optional)
examples: $(LIB) $(LIB_STREAM)
	$(CC) $(CFLAGS) examples/example_basic.c -o examples/example_basic -L./lib -laukadicty
	$(CC) $(CFLAGS) examples/example_load.c -o examples/example_load -L./lib -laukadicty
	$(CC) $(CFLAGS) examples/example_listener.c -o examples/example_listener -L./lib -laukadicty
	$(CC) $(CFLAGS) examples/example_streamcache.c -o examples/example_streamcache -L./lib -laukstreamcache -laukadicty
	@echo "Examples built"

# Clean examples
clean-examples:
	rm -f examples/example_basic examples/example_load examples/example_listener examples/example_streamcache

.PHONY: all clean rebuild examples clean-examples

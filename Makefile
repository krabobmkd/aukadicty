# Makefile for Aukadicty static libraries
# AmigaOS 3.1 compatible

CC = gcc
AR = ar

# Detect OS and set appropriate include paths
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(findstring CYGWIN,$(UNAME_S)),CYGWIN)
    OS_INCLUDE = -I./AmigaStack/include
    BUILD_DIR = build-cygwin
else ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    OS_INCLUDE = -I./AmigaStack/include
    BUILD_DIR = build-mingw
else ifeq ($(findstring Windows,$(UNAME_S)),Windows)
    OS_INCLUDE = -I./AmigaStack/include
    BUILD_DIR = build-windows
else ifeq ($(UNAME_S),Linux)
    OS_INCLUDE = -I./AmigaStack/include
    BUILD_DIR = build-linux
else ifeq ($(UNAME_S),Darwin)
    OS_INCLUDE = -I./AmigaStack/include
    BUILD_DIR = build-macos
else
    # Assume AmigaOS or cross-compilation
    OS_INCLUDE = -I./os-include
    BUILD_DIR = build-amiga
endif

CFLAGS = -Wall -Wextra -std=c99 -O2 -I./include $(OS_INCLUDE) -I./cjson -I./aukstreamcache/include
ARFLAGS = rcs

# Output directories
LIB_DIR = $(BUILD_DIR)/lib
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Library names
LIB = $(LIB_DIR)/libaukadicty.a
LIB_STREAM = $(LIB_DIR)/libaukstreamcache.a

# Aukadicty source files
SRC = src/aukobject.c \
      src/aukmutex.c \
      src/aukarray.c \
      src/aukstring.c \
      src/aukfixed.c \
      src/aukproject.c \
      src/aukaproject.c \
      src/auktrack.c \
      src/auksound.c \
      src/auksoundfile.c \
      src/aukscalararray.c \
      src/aukjson.c \
      src/auktyperegistry.c \
      src/aukjsonserializer.c \
      src/aukiffserializer.c \
      cjson/cJSON.c

# AukStreamCache source files
SRC_STREAM = aukstreamcache/src/aukstream.c \
             aukstreamcache/src/aukstreampool.c \
             aukstreamcache/src/aukstreamwave.c \
             aukstreamcache/src/aukstream8svx.c \
             aukstreamcache/src/aukstreamconvert.c \
             aukstreamcache/src/aukstreamloader.c

# Object files (redirect to build directory)
OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
OBJ_STREAM = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_STREAM))

# Default target - build both libraries
all: $(LIB) $(LIB_STREAM)

# Build aukadicty library
$(LIB): $(OBJ)
	@mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Library built: $(LIB)"

# Build aukstreamcache library
$(LIB_STREAM): $(OBJ_STREAM)
	@mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Library built: $(LIB_STREAM)"

# Compile source files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts in $(BUILD_DIR)"

# Rebuild everything
rebuild: clean all

# Build examples (optional)
examples: $(LIB) $(LIB_STREAM)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) examples/example_basic.c -o $(BIN_DIR)/example_basic -L$(LIB_DIR) -laukadicty
	$(CC) $(CFLAGS) examples/example_load.c -o $(BIN_DIR)/example_load -L$(LIB_DIR) -laukadicty
	$(CC) $(CFLAGS) examples/example_listener.c -o $(BIN_DIR)/example_listener -L$(LIB_DIR) -laukadicty
	$(CC) $(CFLAGS) examples/example_streamcache.c -o $(BIN_DIR)/example_streamcache -L$(LIB_DIR) -laukstreamcache -laukadicty
	$(CC) $(CFLAGS) examples/example_iff.c -o $(BIN_DIR)/example_iff -L$(LIB_DIR) -laukadicty
	@echo "Examples built in $(BIN_DIR)"

# Clean examples
clean-examples:
	rm -rf $(BIN_DIR)
	@echo "Cleaned examples in $(BIN_DIR)"

.PHONY: all clean rebuild examples clean-examples

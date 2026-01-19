# Makefile for FFT test
#
# For Linux testing:
#   make
#
# For AmigaOS with GCC cross-compiler:
#   make CC=m68k-amigaos-gcc CFLAGS="-O2 -m68000 -DAMIGA" LDFLAGS="-noixemul"

CC = gcc
CFLAGS = -Wall -O2
LDFLAGS =

# For Amiga cross-compilation, uncomment:
# CC = m68k-amigaos-gcc
# CFLAGS = -Wall -O2 -m68000 -DAMIGA -noixemul
# LDFLAGS = -noixemul

OBJS = fft_68k.o fft_test.o

all: fft_test

fft_test: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

fft_68k.o: fft_68k.c fft_68k.h
	$(CC) $(CFLAGS) -c -o $@ fft_68k.c

fft_test.o: fft_test.c fft_68k.h
	$(CC) $(CFLAGS) -c -o $@ fft_test.c

clean:
	rm -f *.o fft_test

.PHONY: all clean

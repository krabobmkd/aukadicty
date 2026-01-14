# Makefile for AukProject library - Amiga GCC 2.x
# Use with command "make"
# Place this file in the aukadicty root directory as "makefile"

CC = gcc
AR = ar
BUILDDIR = build-gcc
#  -Wall
CFLAGS = -m68030 -O2 -noixemul -Iinclude -Ios-include -Icjson -Iaukstreamcache/include

HEADERS = \
 include/aukadicty.h include/aukobject.h include/aukmutex.h \
 include/aukarray.h include/aukstring.h include/aukfixed.h \
 include/aukproject.h include/aukaproject.h include/auktrack.h \
 include/auksound.h include/auksoundfile.h include/aukscalararray.h \
 include/aukjson.h include/auktyperegistry.h include/serializer.h \
 include/aukjsonserializer.h include/aukiffserializer.h \
 cjson/cJSON.h

# AukProject library objects
LIBOBJS = \
 $(BUILDDIR)/aukobject.o \
 $(BUILDDIR)/aukmutex.o \
 $(BUILDDIR)/aukarray.o \
 $(BUILDDIR)/aukstring.o \
 $(BUILDDIR)/aukfixed.o \
 $(BUILDDIR)/aukproject.o \
 $(BUILDDIR)/aukaproject.o \
 $(BUILDDIR)/auktrack.o \
 $(BUILDDIR)/auksound.o \
 $(BUILDDIR)/auksoundfile.o \
 $(BUILDDIR)/aukscalararray.o \
 $(BUILDDIR)/aukjson.o \
 $(BUILDDIR)/auktyperegistry.o \
 $(BUILDDIR)/aukjsonserializer.o \
 $(BUILDDIR)/aukiffserializer.o \
 $(BUILDDIR)/cJSON.o

# AukStreamCache library objects
STREAMOBJS = \
 $(BUILDDIR)/aukstream.o \
 $(BUILDDIR)/aukstreampool.o \
 $(BUILDDIR)/aukstreamwave.o \
 $(BUILDDIR)/aukstream8svx.o \
 $(BUILDDIR)/aukstreamconvert.o \
 $(BUILDDIR)/aukstreamloader.o

all: $(BUILDDIR)/libaukproject.a $(BUILDDIR)/libaukstreamcache.a

$(BUILDDIR):
	-makedir $(BUILDDIR)

# AukProject library source files
$(BUILDDIR)/aukobject.o: src/aukobject.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukmutex.o: src/aukmutex.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukarray.o: src/aukarray.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukstring.o: src/aukstring.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukfixed.o: src/aukfixed.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukproject.o: src/aukproject.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukaproject.o: src/aukaproject.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/auktrack.o: src/auktrack.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/auksound.o: src/auksound.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/auksoundfile.o: src/auksoundfile.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukscalararray.o: src/aukscalararray.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukjson.o: src/aukjson.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/auktyperegistry.o: src/auktyperegistry.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukjsonserializer.o: src/aukjsonserializer.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukiffserializer.o: src/aukiffserializer.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/cJSON.o: cjson/cJSON.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

# AukStreamCache library source files
$(BUILDDIR)/aukstream.o: aukstreamcache/src/aukstream.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukstreampool.o: aukstreamcache/src/aukstreampool.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukstreamwave.o: aukstreamcache/src/aukstreamwave.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukstream8svx.o: aukstreamcache/src/aukstream8svx.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukstreamconvert.o: aukstreamcache/src/aukstreamconvert.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/aukstreamloader.o: aukstreamcache/src/aukstreamloader.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Create static libraries
$(BUILDDIR)/libaukproject.a: $(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)

$(BUILDDIR)/libaukstreamcache.a: $(STREAMOBJS)
	$(AR) rcs $@ $(STREAMOBJS)

clean:
	-delete ALL FORCE $(BUILDDIR)

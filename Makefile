# Makefile for AukProject library - Amiga GCC 2.x
# Use with command "make"
# Place this file in the aukadicty root directory as "makefile"

CC = gcc
AR = ar
BUILDDIRAUK = build-auk-gcc
BUILDDIRSTRCA = build-stc-gcc
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
 $(BUILDDIRAUK)/aukobject.o \
 $(BUILDDIRAUK)/aukmutex.o \
 $(BUILDDIRAUK)/aukarray.o \
 $(BUILDDIRAUK)/aukstring.o \
 $(BUILDDIRAUK)/aukfixed.o \
 $(BUILDDIRAUK)/aukproject.o \
 $(BUILDDIRAUK)/aukaproject.o \
 $(BUILDDIRAUK)/auktrack.o \
 $(BUILDDIRAUK)/auksound.o \
 $(BUILDDIRAUK)/auksoundfile.o \
 $(BUILDDIRAUK)/aukscalararray.o \
 $(BUILDDIRAUK)/auktyperegistry.o \
 $(BUILDDIRAUK)/aukiffserializer.o 

# $(BUILDDIRAUK)/aukjsonserializer.o
# $(BUILDDIRAUK)/aukjson.o 
# $(BUILDDIRAUK)/cJSON.o

# AukStreamCache library objects
STREAMLIBOBJS = \
 $(BUILDDIRSTRCA)/aukstream.o \
 $(BUILDDIRSTRCA)/aukstreampool.o \
 $(BUILDDIRSTRCA)/aukstreamwave.o \
 $(BUILDDIRSTRCA)/aukstream8svx.o \
 $(BUILDDIRSTRCA)/aukstreamconvert.o \
 $(BUILDDIRSTRCA)/aukstreamloader.o

all: $(BUILDDIRAUK)/libaukproject.a $(BUILDDIRSTRCA)/libaukstreamcache.a

$(BUILDDIRAUK):
	-makedir $(BUILDDIRAUK)

$(BUILDDIRSTRCA):
	-makedir $(BUILDDIRSTRCA)

# AukProject library source files
$(BUILDDIRAUK)/%.o: src/%.c $(BUILDDIRAUK) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIRSTRCA)/%.o: aukstreamcache/src/%.c $(BUILDDIRSTRCA) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Create static libraries
$(BUILDDIRAUK)/libaukproject.a: $(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)

$(BUILDDIRSTRCA)/libaukstreamcache.a: $(STREAMLIBOBJS)
	$(AR) rcs $@ $(STREAMOBJS)

clean:
	-delete ALL FORCE $(BUILDDIR)

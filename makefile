# Makefile for AukProject library - Amiga GCC 2.x
# Use with command "make"
# Place this file in the aukadicty root directory as "makefile"

MCPU ?= m68030
USEJSON = 1
CC = gcc
AR = ar
BUILDDIRAUK = build-gcc-$(MCPU)
BUILDDIRSTRCA = build-stc-gcc-$(MCPU)
BUILDDIRJSON = build-json-gcc-$(MCPU)
#  -Wall
CFLAGS = -$(MCPU) -O2 -noixemul -Iinclude -Ios-include -Icjson -Iaukstreamcache/include

HEADERS = \
 include/aukadicty.h include/aukobject.h include/aukmutex.h \
 include/aukarray.h include/aukstring.h include/aukfixed.h \
 include/aukproject.h include/aukaproject.h include/auktrack.h \
 include/auksound.h include/auksoundfile.h include/aukscalararray.h \
 include/auktyperegistry.h include/serializer.h \
 include/aukiffserializer.h

#ifeq ($(USEJSON), 1 )
HEADERS +=  cjson/cJSON.h include/aukjson.h sinclude/aukjsonserializer.h
#endif
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

#ifeq ($(USEJSON), 1 )
LIBOBJS += \
        $(BUILDDIRAUK)/aukjsonserializer.o \
        $(BUILDDIRAUK)/aukjson.o

JSONLIBOBJS = \
        $(BUILDDIRJSON)/cJSON.o

#endif

# AukStreamCache library objects
STREAMLIBOBJS = \
 $(BUILDDIRSTRCA)/aukstream.o \
 $(BUILDDIRSTRCA)/aukstreampool.o \
 $(BUILDDIRSTRCA)/aukstreamwave.o \
 $(BUILDDIRSTRCA)/aukstream8svx.o \
 $(BUILDDIRSTRCA)/aukstreamconvert.o \
 $(BUILDDIRSTRCA)/aukstreamloader.o

all: $(BUILDDIRAUK)/libaukproject$(MCPU).a $(BUILDDIRSTRCA)/libaukstreamcache.a

$(BUILDDIRAUK):
	#-makedir $(BUILDDIRAUK)
	-mkdir -p $(BUILDDIRAUK)

$(BUILDDIRSTRCA):
	-makedir $(BUILDDIRSTRCA)

$(BUILDDIRJSON):
	-makedir $(BUILDDIRJSON)

# AukProject library source files
$(BUILDDIRAUK)/%.o: src/%.c $(BUILDDIRAUK) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIRSTRCA)/%.o: aukstreamcache/src/%.c $(BUILDDIRSTRCA) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIRJSON)/%.o: cjson/%.c $(BUILDDIRJSON)
	$(CC) -c -o $@ $< $(CFLAGS)

# Create static libraries
$(BUILDDIRAUK)/libaukproject$(MCPU).a: $(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)

$(BUILDDIRSTRCA)/libaukstreamcache.a: $(STREAMLIBOBJS)
	$(AR) rcs $@ $(STREAMOBJS)

$(BUILDDIRJSON)/libjson.a: $(JSONLIBOBJS)
	$(AR) rcs $@ $(JSONLIBOBJS)

clean:
	-delete ALL FORCE $(BUILDDIRAUK)

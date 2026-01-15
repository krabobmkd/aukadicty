# Makefile for AukBoopsi - Amiga GCC 2.95
# Use with command "make"

CC = gcc
BUILDDIR = build-gcc
BUILDDIRIS = $(BUILDDIR)/is
BUILDDIRTA = $(BUILDDIR)/ta
BUILDDIRTLA = $(BUILDDIR)/tla
BUILDDIRTR = $(BUILDDIR)/tr
BUILDDIRTH = $(BUILDDIR)/th
BUILDDIRVOLR = $(BUILDDIR)/vr

STATICDEFS = -DTRACKLIST_STATICLINK=1 -DINFINITESCROLL_STATICLINK=1 -DUSE_DEBUG_BDBPRINT=1

# -I../include
CFLAGS = -m68030 -O2 -noixemul -Wall -ITrackArea -I. $(STATICDEFS)
LIBS = ../build-gcc/libaukproject.a

HEADERS = \
 aukstylesheet.h aukstyle.h gadgetid.h bdbprintf.h \
 TrackListView.h HeaderView.h FooterView.h \
 auklocale.h aukaction.h aukmenu.h offscreenbm.h \
 InfiniteScroll/class_infinitescroll.h InfiniteScroll/class_infinitescroll_private.h \
 TrackArea/class_trackarea.h TrackArea/class_trackarea_private.h \
 TrackListArea/class_tracklistarea.h TrackListArea/class_tracklistarea_private.h \
 TimeRule/class_timerule.h TimeRule/class_timerule_private.h \
 TrackHeader/class_trackheader.h TrackHeader/class_trackheader_private.h \
 VolumeRule/class_volumerule.h VolumeRule/class_volumerule_private.h

OBJS = \
 $(BUILDDIR)/aukboopsi.o \
 $(BUILDDIR)/TrackListView.o \
 $(BUILDDIR)/HeaderView.o \
 $(BUILDDIR)/FooterView.o \
 $(BUILDDIR)/auklocale.o \
 $(BUILDDIR)/aukaction.o \
 $(BUILDDIR)/aukmenu.o \
 $(BUILDDIR)/aukstylesheet.o \
 $(BUILDDIR)/bdbprintf.o \
 $(BUILDDIR)/offscreenbm.o \
 $(BUILDDIRIS)/class_infinitescroll_attribs.o \
 $(BUILDDIRIS)/class_infinitescroll_dispatch.o \
 $(BUILDDIRIS)/class_infinitescroll_lib.o \
 $(BUILDDIRIS)/class_infinitescroll_render.o \
 $(BUILDDIRIS)/class_infinitescroll_handleinput.o \
 $(BUILDDIRTA)/class_trackarea_attribs.o \
 $(BUILDDIRTA)/class_trackarea_dispatch.o \
 $(BUILDDIRTA)/class_trackarea_lib.o \
 $(BUILDDIRTA)/class_trackarea_render.o \
 $(BUILDDIRTA)/class_trackarea_handleinput.o \
 $(BUILDDIRTLA)/class_tracklistarea_attribs.o \
 $(BUILDDIRTLA)/class_tracklistarea_dispatch.o \
 $(BUILDDIRTLA)/class_tracklistarea_lib.o \
 $(BUILDDIRTLA)/class_tracklistarea_render.o \
 $(BUILDDIRTLA)/class_tracklistarea_handleinput.o \
 $(BUILDDIRTR)/class_timerule_attribs.o \
 $(BUILDDIRTR)/class_timerule_dispatch.o \
 $(BUILDDIRTR)/class_timerule_lib.o \
 $(BUILDDIRTR)/class_timerule_render.o \
 $(BUILDDIRTR)/class_timerule_handleinput.o \
 $(BUILDDIRTH)/class_trackheader_attribs.o \
 $(BUILDDIRTH)/class_trackheader_dispatch.o \
 $(BUILDDIRTH)/class_trackheader_render.o \
 $(BUILDDIRTH)/class_trackheader_lib.o \
 $(BUILDDIRVOLR)/class_volumerule_attribs.o \
 $(BUILDDIRVOLR)/class_volumerule_dispatch.o \
 $(BUILDDIRVOLR)/class_volumerule_lib.o \
 $(BUILDDIRVOLR)/class_volumerule_render.o

all: $(BUILDDIR)/aukadicty

$(BUILDDIR):
	-makedir $(BUILDDIR)

$(BUILDDIRIS):
	-makedir $(BUILDDIRIS)

$(BUILDDIRTA):
	-makedir $(BUILDDIRTA)

$(BUILDDIRTLA):
	-makedir $(BUILDDIRTLA)

$(BUILDDIRTR):
	-makedir $(BUILDDIRTR)

$(BUILDDIRTH):
	-makedir $(BUILDDIRTH)

$(BUILDDIRVOLR):
	-makedir $(BUILDDIRVOLR)

# Main source files
$(BUILDDIR)/%.o: %.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -I../include

$(BUILDDIRIS)/%.o: InfiniteScroll/%.c $(BUILDDIRIS) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -I../../include

$(BUILDDIRTA)/%.o: TrackArea/%.c $(BUILDDIRTA) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -I../../include

$(BUILDDIRTLA)/%.o: TrackListArea/%.c $(BUILDDIRTLA) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -I../../include

$(BUILDDIRTR)/%.o: TimeRule/%.c $(BUILDDIRTR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -I../../include

$(BUILDDIRTH)/%.o: TrackHeader/%.c $(BUILDDIRTH) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -I../../include

$(BUILDDIRVOLR)/%.o: VolumeRule/%.c $(BUILDDIRVOLR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -I../../include

# Link executable
$(BUILDDIR)/aukadicty: $(OBJS) $(LIBS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(CFLAGS) -s

clean:
	-delete ALL FORCE $(BUILDDIR)

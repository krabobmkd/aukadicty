# Makefile for AukBoopsi - Amiga GCC 2.x
# Use with command "make"

CC = gcc
BUILDDIR = build-gcc

CFLAGS = -m68030 -O2 -noixemul -Wall -Wextra -I../include -ITrackArea -I.
STATICDEFS = -DTRACKAREA_STATICLINK=1 -DTRACKHEADER_STATICLINK=1 -DTIMERULE_STATICLINK=1 -DTRACKLIST_STATICLINK=1 -DINFINITESCROLL_STATICLINK=1 -DVOLUMERULE_STATICLINK=1 -DUSE_DEBUG_BDBPRINT=1

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
 $(BUILDDIR)/class_infinitescroll_attribs.o \
 $(BUILDDIR)/class_infinitescroll_dispatch.o \
 $(BUILDDIR)/class_infinitescroll_lib.o \
 $(BUILDDIR)/class_infinitescroll_render.o \
 $(BUILDDIR)/class_infinitescroll_handleinput.o \
 $(BUILDDIR)/class_trackarea_attribs.o \
 $(BUILDDIR)/class_trackarea_dispatch.o \
 $(BUILDDIR)/class_trackarea_lib.o \
 $(BUILDDIR)/class_trackarea_render.o \
 $(BUILDDIR)/class_trackarea_handleinput.o \
 $(BUILDDIR)/class_tracklistarea_attribs.o \
 $(BUILDDIR)/class_tracklistarea_dispatch.o \
 $(BUILDDIR)/class_tracklistarea_lib.o \
 $(BUILDDIR)/class_tracklistarea_render.o \
 $(BUILDDIR)/class_tracklistarea_handleinput.o \
 $(BUILDDIR)/class_timerule_attribs.o \
 $(BUILDDIR)/class_timerule_dispatch.o \
 $(BUILDDIR)/class_timerule_lib.o \
 $(BUILDDIR)/class_timerule_render.o \
 $(BUILDDIR)/class_timerule_handleinput.o \
 $(BUILDDIR)/class_trackheader_attribs.o \
 $(BUILDDIR)/class_trackheader_dispatch.o \
 $(BUILDDIR)/class_trackheader_render.o \
 $(BUILDDIR)/class_trackheader_lib.o \
 $(BUILDDIR)/class_volumerule_attribs.o \
 $(BUILDDIR)/class_volumerule_dispatch.o \
 $(BUILDDIR)/class_volumerule_lib.o \
 $(BUILDDIR)/class_volumerule_render.o

all: $(BUILDDIR)/aukadicty

$(BUILDDIR):
	-makedir $(BUILDDIR)

# Main source files
$(BUILDDIR)/aukboopsi.o: aukboopsi.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/TrackListView.o: TrackListView.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/HeaderView.o: HeaderView.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/FooterView.o: FooterView.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/auklocale.o: auklocale.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/aukaction.o: aukaction.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/aukmenu.o: aukmenu.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/aukstylesheet.o: aukstylesheet.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/bdbprintf.o: bdbprintf.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/offscreenbm.o: offscreenbm.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

# InfiniteScroll class
$(BUILDDIR)/class_infinitescroll_attribs.o: InfiniteScroll/class_infinitescroll_attribs.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_infinitescroll_dispatch.o: InfiniteScroll/class_infinitescroll_dispatch.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_infinitescroll_lib.o: InfiniteScroll/class_infinitescroll_lib.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_infinitescroll_render.o: InfiniteScroll/class_infinitescroll_render.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_infinitescroll_handleinput.o: InfiniteScroll/class_infinitescroll_handleinput.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

# TrackArea class
$(BUILDDIR)/class_trackarea_attribs.o: TrackArea/class_trackarea_attribs.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_trackarea_dispatch.o: TrackArea/class_trackarea_dispatch.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_trackarea_lib.o: TrackArea/class_trackarea_lib.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_trackarea_render.o: TrackArea/class_trackarea_render.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_trackarea_handleinput.o: TrackArea/class_trackarea_handleinput.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

# TrackListArea class
$(BUILDDIR)/class_tracklistarea_attribs.o: TrackListArea/class_tracklistarea_attribs.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_tracklistarea_dispatch.o: TrackListArea/class_tracklistarea_dispatch.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_tracklistarea_lib.o: TrackListArea/class_tracklistarea_lib.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_tracklistarea_render.o: TrackListArea/class_tracklistarea_render.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_tracklistarea_handleinput.o: TrackListArea/class_tracklistarea_handleinput.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

# TimeRule class
$(BUILDDIR)/class_timerule_attribs.o: TimeRule/class_timerule_attribs.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_timerule_dispatch.o: TimeRule/class_timerule_dispatch.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_timerule_lib.o: TimeRule/class_timerule_lib.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_timerule_render.o: TimeRule/class_timerule_render.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_timerule_handleinput.o: TimeRule/class_timerule_handleinput.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

# TrackHeader class
$(BUILDDIR)/class_trackheader_attribs.o: TrackHeader/class_trackheader_attribs.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_trackheader_dispatch.o: TrackHeader/class_trackheader_dispatch.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_trackheader_render.o: TrackHeader/class_trackheader_render.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_trackheader_lib.o: TrackHeader/class_trackheader_lib.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

# VolumeRule class
$(BUILDDIR)/class_volumerule_attribs.o: VolumeRule/class_volumerule_attribs.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_volumerule_dispatch.o: VolumeRule/class_volumerule_dispatch.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_volumerule_lib.o: VolumeRule/class_volumerule_lib.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

$(BUILDDIR)/class_volumerule_render.o: VolumeRule/class_volumerule_render.c $(BUILDDIR) $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(STATICDEFS)

# Link executable
$(BUILDDIR)/aukadicty: $(OBJS) $(LIBS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(CFLAGS) -s

clean:
	-delete ALL FORCE $(BUILDDIR)

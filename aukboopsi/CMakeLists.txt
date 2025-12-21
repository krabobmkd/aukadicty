cmake_minimum_required(VERSION 3.10)
project(AukBoopsi C)


# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-Os")

add_subdirectory(../ AukProject)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/TrackGadget
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# AukApp source files
set(AUKBOOPSI_SOURCES
    aukboopsi.c
    TrackListUi.c
    TrackListUi.h

    bdbprintf.c
    bdbprintf.h

    # track gadget (private)
    TrackGadget/class_track_attribs.c
    TrackGadget/class_track_dispatch.c
    TrackGadget/class_track_lib.c
    TrackGadget/class_track_render.c
    TrackGadget/class_track_handleinput.c
    TrackGadget/class_track_private.h
    TrackGadget/class_track.h

    TrackListAreaUi/class_tracklistareaui_attribs.c
    TrackListAreaUi/class_tracklistareaui_dispatch.c
    TrackListAreaUi/class_tracklistareaui_lib.c
    TrackListAreaUi/class_tracklistareaui_render.c
    TrackListAreaUi/class_tracklistareaui_handleinput.c
    TrackListAreaUi/class_tracklistareaui_private.h
    TrackListAreaUi/class_tracklistareaui.h

    TimeRule/class_timerule_attribs.c
    TimeRule/class_timerule_dispatch.c
    TimeRule/class_timerule_lib.c
    TimeRule/class_timerule_render.c
    TimeRule/class_timerule_handleinput.c
    TimeRule/class_timerule_private.h
    TimeRule/class_timerule.h

    TrackHeader/class_trackheader_attribs.c
    TrackHeader/class_trackheader_dispatch.c
    TrackHeader/class_trackheader_lib.c
    TrackHeader/class_trackheader_render.c
    TrackHeader/class_trackheader_handleinput.c
    TrackHeader/class_trackheader_private.h
    TrackHeader/class_trackheader.h

    TrackHeaderList/class_trackheaderlist_attribs.c
    TrackHeaderList/class_trackheaderlist_dispatch.c
    TrackHeaderList/class_trackheaderlist_lib.c
    TrackHeaderList/class_trackheaderlist_render.c
    TrackHeaderList/class_trackheaderlist_handleinput.c
    TrackHeaderList/class_trackheaderlist_private.h
    TrackHeaderList/class_trackheaderlist.h


)

# Create app
add_executable(aukadicty  ${AUKBOOPSI_SOURCES} )

# Install targets
install(TARGETS aukadicty
    ARCHIVE DESTINATION bin
)

target_link_libraries(aukadicty aukproject)
target_compile_definitions(aukadicty PRIVATE
			TRACK_STATICLINK=1
			TRACKHEADER_STATICLINK=1
			TIMERULE_STATICLINK=1
			TRACKLIST_STATICLINK=1
			TRACKHEADERLIST_STATICLINK=1
			USE_DEBUG_BDBPRINT=1)


# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)




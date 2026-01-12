cmake_minimum_required(VERSION 3.10)
project(AukBoopsi C)


# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(AMIGA_USE_FLOAT "use any float or double" OFF)
option(AMIGA_USE_HARDFLOAT "compile using direct 68882 FPU, will not work on economic 060, off means soft float" OFF)
option(AMIGA_USE_68030 " use 68030" ON)
option(AMIGA_USE_68040 " use 68040" OFF)
option(AMIGA_USE_68060 " use 68060" OFF)


#  -fno-rtti -fno-exceptions -fno-strict-aliasing
	# add your generic options here:
	#add_compile_options( -save-temps )
	# this is to have assember .s files in the build.
	list(APPEND AMIGA_COMPILER_OPTIONS -save-temps )

	if(AMIGA_USE_68030)
		list(APPEND AMIGA_COMPILER_OPTIONS -m68030)
	elseif(AMIGA_USE_68040)
		list(APPEND AMIGA_COMPILER_OPTIONS -m68040)
	elseif(AMIGA_USE_68060)
		list(APPEND AMIGA_COMPILER_OPTIONS -m68060)
	else()
		list(APPEND AMIGA_COMPILER_OPTIONS -m68020)
	endif()
	if(AMIGA_USE_FLOAT)
		if(AMIGA_USE_HARDFLOAT)
			list(APPEND AMIGA_COMPILER_OPTIONS -mhard-float -ffast-math)
		else()
			list(APPEND AMIGA_COMPILER_OPTIONS -msoft-float)
		endif()
	endif()


# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-Os")

add_subdirectory(../ AukProject)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/TrackArea
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# AukApp source files
set(AUKBOOPSI_SOURCES
    aukboopsi.c
    TrackListView.c
    TrackListView.h
    HeaderView.c
    HeaderView.h
    FooterView.c
    FooterView.h

    auklocale.c
    auklocale.h

    aukaction.c
    aukaction.h

    aukmenu.c
    aukmenu.h

    aukstylesheet.c
    aukstylesheet.h
    gadgetid.h

    bdbprintf.c
    bdbprintf.h

    offscreenbm.c
    offscreenbm.h

    # infinite scroll base class (private)
    InfiniteScroll/class_infinitescroll_attribs.c
    InfiniteScroll/class_infinitescroll_dispatch.c
    InfiniteScroll/class_infinitescroll_lib.c
    InfiniteScroll/class_infinitescroll_render.c
    InfiniteScroll/class_infinitescroll_handleinput.c
    InfiniteScroll/class_infinitescroll_private.h
    InfiniteScroll/class_infinitescroll.h

    # track area gadget (private)
    TrackArea/class_trackarea_attribs.c
    TrackArea/class_trackarea_dispatch.c
    TrackArea/class_trackarea_lib.c
    TrackArea/class_trackarea_render.c
    TrackArea/class_trackarea_handleinput.c
    TrackArea/class_trackarea_private.h
    TrackArea/class_trackarea.h

    TrackListArea/class_tracklistarea_attribs.c
    TrackListArea/class_tracklistarea_dispatch.c
    TrackListArea/class_tracklistarea_lib.c
    TrackListArea/class_tracklistarea_render.c
    TrackListArea/class_tracklistarea_handleinput.c
    TrackListArea/class_tracklistarea_private.h
    TrackListArea/class_tracklistarea.h

    TimeRule/class_timerule_attribs.c
    TimeRule/class_timerule_dispatch.c
    TimeRule/class_timerule_lib.c
    TimeRule/class_timerule_render.c
    TimeRule/class_timerule_handleinput.c
    TimeRule/class_timerule_private.h
    TimeRule/class_timerule.h

    TrackHeader/class_trackheader_attribs.c
    TrackHeader/class_trackheader_dispatch.c
    TrackHeader/class_trackheader_render.c
    TrackHeader/class_trackheader_lib.c
    TrackHeader/class_trackheader_private.h
    TrackHeader/class_trackheader.h

    VolumeRule/class_volumerule_attribs.c
    VolumeRule/class_volumerule_dispatch.c
    VolumeRule/class_volumerule_lib.c
    VolumeRule/class_volumerule_render.c
    VolumeRule/class_volumerule_private.h
    VolumeRule/class_volumerule.h

)

# Create app
add_executable(aukadicty  ${AUKBOOPSI_SOURCES} )

# Install targets
install(TARGETS aukadicty
    ARCHIVE DESTINATION bin
)

target_link_libraries(aukadicty aukproject)
target_compile_definitions(aukadicty PRIVATE
			TRACKAREA_STATICLINK=1
			TRACKHEADER_STATICLINK=1
			TIMERULE_STATICLINK=1
			TRACKLIST_STATICLINK=1
			INFINITESCROLL_STATICLINK=1
			VOLUMERULE_STATICLINK=1
			USE_DEBUG_BDBPRINT=1)
target_compile_options(aukadicty PRIVATE
		$<$<CONFIG:RELEASE>:-O2 -Wno-narrowing -DNDEBUG>
		 ${AMIGA_COMPILER_OPTIONS} )


# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)




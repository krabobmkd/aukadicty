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
    tracklayout.c
    tracklayout.h

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
)

# Create app
add_executable(aukadicty  ${AUKBOOPSI_SOURCES} )

# Install targets
install(TARGETS aukadicty
    ARCHIVE DESTINATION bin
)

target_link_libraries(aukadicty aukproject)
target_compile_definitions(aukadicty PRIVATE TRACK_STATICLINK=1 USE_DEBUG_BDBPRINT=1)


# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)




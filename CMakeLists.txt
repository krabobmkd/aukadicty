cmake_minimum_required(VERSION 3.10)
project(Aukadicty C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2")

# Detect platform and set AmigaStack for non-Amiga builds
if(NOT CMAKE_SYSTEM_NAME MATCHES "Amiga")
    set(USE_AMIGA_STACK ON)
    message(STATUS "Using AmigaStack compatibility layer for ${CMAKE_SYSTEM_NAME}")
else()
    set(USE_AMIGA_STACK OFF)
    message(STATUS "Native AmigaOS build - no compatibility layer needed")
endif()

# Include directories
if(USE_AMIGA_STACK)
    include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/AmigaStack/include
        ${CMAKE_SOURCE_DIR}/cjson
        ${CMAKE_SOURCE_DIR}/aukstreamcache/include
    )
else()
    include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/os-include
        ${CMAKE_SOURCE_DIR}/cjson
        ${CMAKE_SOURCE_DIR}/aukstreamcache/include
    )
endif()

# Aukadicty source files
set(AUKADICTY_SOURCES
    src/aukobject.c
    src/aukmutex.c
    src/aukarray.c
    src/aukstring.c
    src/aukfixed.c
    src/aukproject.c
    src/auktrack.c
    src/auksound.c
    src/auksoundfile.c
    src/aukjson.c
    src/auktyperegistry.c
    cjson/cJSON.c
)

# AmigaStack source files (for non-Amiga platforms)
if(USE_AMIGA_STACK)
    set(AMIGASTACK_SOURCES
        AmigaStack/src/exec.c
        AmigaStack/src/dos.c
    )
endif()

# AukStreamCache source files
set(AUKSTREAMCACHE_SOURCES
    aukstreamcache/src/aukstream.c
    aukstreamcache/src/aukstreampool.c
    aukstreamcache/src/aukstreamwave.c
    aukstreamcache/src/aukstream8svx.c
    aukstreamcache/src/aukstreamconvert.c
    aukstreamcache/src/aukstreamloader.c
)

# Aukadicty header files
set(AUKADICTY_HEADERS
    include/aukadicty.h
    include/aukobject.h
    include/aukmutex.h
    include/aukarray.h
    include/aukstring.h
    include/aukfixed.h
    include/aukproject.h
    include/auktrack.h
    include/auksound.h
    include/auksoundfile.h
    include/aukjson.h
    include/auktyperegistry.h
    include/serializer.h
    cjson/cJSON.h
)

# AukStreamCache header files
set(AUKSTREAMCACHE_HEADERS
    aukstreamcache/include/aukstream.h
    aukstreamcache/include/aukstreamtypes.h
    aukstreamcache/include/aukstreampool.h
)

# Create aukadicty static library
if(USE_AMIGA_STACK)
    add_library(aukadicty STATIC ${AUKADICTY_SOURCES} ${AMIGASTACK_SOURCES})
else()
    add_library(aukadicty STATIC ${AUKADICTY_SOURCES})
endif()
set_target_properties(aukadicty PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Create aukstreamcache static library
if(USE_AMIGA_STACK)
    add_library(aukstreamcache STATIC ${AUKSTREAMCACHE_SOURCES} ${AMIGASTACK_SOURCES})
else()
    add_library(aukstreamcache STATIC ${AUKSTREAMCACHE_SOURCES})
endif()
target_link_libraries(aukstreamcache aukadicty)
set_target_properties(aukstreamcache PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Add pthread library for POSIX systems
if(USE_AMIGA_STACK AND UNIX AND NOT APPLE)
    target_link_libraries(aukadicty pthread)
    target_link_libraries(aukstreamcache pthread)
endif()

# Install targets
install(TARGETS aukadicty aukstreamcache
    ARCHIVE DESTINATION lib
)

install(FILES ${AUKADICTY_HEADERS}
    DESTINATION include/aukadicty
)

install(FILES ${AUKSTREAMCACHE_HEADERS}
    DESTINATION include/aukstreamcache
)

# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    # Example: Basic usage
    add_executable(example_basic examples/example_basic.c)
    target_link_libraries(example_basic aukadicty)
    set_target_properties(example_basic PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: Load project
    add_executable(example_load examples/example_load.c)
    target_link_libraries(example_load aukadicty)
    set_target_properties(example_load PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: Listener pattern
    add_executable(example_listener examples/example_listener.c)
    target_link_libraries(example_listener aukadicty)
    set_target_properties(example_listener PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: Stream cache
    add_executable(example_streamcache examples/example_streamcache.c)
    target_link_libraries(example_streamcache aukstreamcache aukadicty)
    set_target_properties(example_streamcache PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    message(STATUS "Building examples: Yes")
else()
    message(STATUS "Building examples: No (use -DBUILD_EXAMPLES=ON to enable)")
endif()

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Libraries output: ${CMAKE_BINARY_DIR}/lib/")
message(STATUS "  - libaukadicty.a")
message(STATUS "  - libaukstreamcache.a")

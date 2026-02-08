cmake_minimum_required(VERSION 3.10)
project(AukProject C)

option(AMIGA_USE_FLOAT "use any float or double" OFF)
option(AMIGA_USE_HARDFLOAT "compile using direct 68882 FPU, will not work on economic 060, off means soft float" OFF)
option(AMIGA_USE_68030 " use 68030" ON)
option(AMIGA_USE_68040 " use 68040" OFF)
option(AMIGA_USE_68060 " use 68060" OFF)

option(ENABLEJSONSERIAL " embbed json lib" ON)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags - conditional for MSVC vs GCC/Clang
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
endif()
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2")

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

# Detect platform and set AmigaStack for non-Amiga builds
if(NOT AMIGA)
    set(USE_AMIGA_STACK ON)
    message(STATUS "Using AmigaStack compatibility layer for ${CMAKE_SYSTEM_NAME}")
else()
    set(USE_AMIGA_STACK OFF)
    message(STATUS "Native AmigaOS build - no compatibility layer needed")
endif()

# Include directories
if(USE_AMIGA_STACK)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/AmigaStack/include
        ${CMAKE_CURRENT_SOURCE_DIR}/cjson
        ${CMAKE_CURRENT_SOURCE_DIR}/aukstreamcache/include
    )
else()
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/os-include
        ${CMAKE_CURRENT_SOURCE_DIR}/cjson
        ${CMAKE_CURRENT_SOURCE_DIR}/aukstreamcache/include
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
    src/aukaproject.c
    src/auktrack.c
    src/auksound.c
    src/auksoundfile.c
    src/auksoundfileengine.c
    src/auksoundfile_plugins.c
    src/aukscalararray.c
    src/auktyperegistry.c
    src/aukiffserializer.c
)

if(ENABLEJSONSERIAL)
    list(APPEND AUKADICTY_SOURCES
        src/aukjson.c
        src/aukjsonserializer.c
        cjson/cJSON.c
    )
endif()

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
    include/aukdefs.h
    include/aukadicty.h
    include/aukobject.h
    include/aukmutex.h
    include/aukarray.h
    include/aukstring.h
    include/aukfixed.h
    include/aukproject.h
    include/aukaproject.h
    include/auktrack.h
    include/auksound.h
    include/auksoundfile.h
    include/aukscalararray.h
    include/auktyperegistry.h
    include/serializer.h
    include/aukiffserializer.h
    include/aukselection.h
    include/auksoundfileengine.h
)
if(ENABLEJSONSERIAL)
    list(APPEND AUKADICTY_HEADERS
        include/aukjson.h
        include/aukjsonserializer.h
        cjson/cJSON.h
    )
endif()
# AukStreamCache header files
set(AUKSTREAMCACHE_HEADERS
    aukstreamcache/include/aukstream.h
    aukstreamcache/include/aukstreamtypes.h
    aukstreamcache/include/aukstreampool.h
)

# Create aukadicty static library
if(USE_AMIGA_STACK)
    add_library(aukproject STATIC ${AUKADICTY_SOURCES} ${AMIGASTACK_SOURCES})
else()
    add_library(aukproject STATIC ${AUKADICTY_SOURCES})
    target_compile_options(aukproject PRIVATE
            $<$<CONFIG:RELEASE>:-O2 -Wno-narrowing -DNDEBUG>
             ${AMIGA_COMPILER_OPTIONS} )

endif()
set_target_properties(aukproject PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Create aukstreamcache static library
if(USE_AMIGA_STACK)
    add_library(aukstreamcache STATIC ${AUKSTREAMCACHE_SOURCES} ${AMIGASTACK_SOURCES})
else()
    add_library(aukstreamcache STATIC ${AUKSTREAMCACHE_SOURCES})
endif()
target_link_libraries(aukstreamcache aukproject)
set_target_properties(aukstreamcache PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Add pthread library for POSIX systems
if(USE_AMIGA_STACK AND UNIX AND NOT APPLE)
#    target_link_libraries(aukadicty pthread)
    target_link_libraries(aukstreamcache pthread)
endif()

# Install targets
install(TARGETS aukproject aukstreamcache
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

# Thread library - pthread on Unix, native on Windows
if(WIN32)
    set(THREAD_LIBS "")
elseif(UNIX)
    set(THREAD_LIBS pthread)
endif()

if(BUILD_EXAMPLES)
    # Example: Basic usage
    add_executable(example_basic examples/example_basic.c)
    target_link_libraries(example_basic aukproject ${THREAD_LIBS})
    set_target_properties(example_basic PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: Load project
    add_executable(example_load examples/example_load.c)
    target_link_libraries(example_load aukproject ${THREAD_LIBS})
    set_target_properties(example_load PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: Listener pattern
    add_executable(example_listener examples/example_listener.c)
    target_link_libraries(example_listener aukproject ${THREAD_LIBS})
    set_target_properties(example_listener PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: Stream cache
    add_executable(example_streamcache examples/example_streamcache.c)
    target_link_libraries(example_streamcache aukstreamcache aukproject)
    set_target_properties(example_streamcache PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: IFF serialization
    add_executable(example_iff examples/example_iff.c)
    target_link_libraries(example_iff aukproject ${THREAD_LIBS})
    set_target_properties(example_iff PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Example: Sound file engine (threading test)
    add_executable(example_soundfileengine examples/example_soundfileengine.c)
    target_link_libraries(example_soundfileengine aukproject ${THREAD_LIBS})
    set_target_properties(example_soundfileengine PROPERTIES
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

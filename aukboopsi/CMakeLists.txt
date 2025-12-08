cmake_minimum_required(VERSION 3.10)
project(AukBoopsi C)


# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O2")

add_subdirectory(../ AukProject)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# AukApp source files
set(AUKBOOPSI_SOURCES
    aukboopsi.c
)


# Aukadicty header files
set(AUKBOOPSI_HEADERS

)

# Create app
add_executable(aukadicty  ${AUKBOOPSI_SOURCES} ${AUKBOOPSI_HEADERS})

# Install targets
install(TARGETS aukadicty
    ARCHIVE DESTINATION bin
)

target_link_libraries(aukadicty aukproject)

# Optional: Build examples
option(BUILD_EXAMPLES "Build example programs" ON)




# Building Aukadicty

This document describes how to build the Aukadicty library and examples using both Make and CMake build systems.

## Prerequisites

- C99-compatible compiler (GCC, Clang, or AmigaOS native compiler)
- Make (for Makefile build)
- CMake 3.10+ (for CMake build)

## Build Systems

The project provides two parallel build systems:

1. **Makefile** - Traditional GNU Make (simpler, direct)
2. **CMakeLists.txt** - CMake (better for cross-compilation, IDEs)

Both build systems are kept in sync and produce identical output.

## Using Make

### Build Library

```bash
make
```

Output: `lib/libaukadicty.a`

### Build Examples

```bash
make examples
```

Output:
- `examples/example_basic`
- `examples/example_load`
- `examples/example_listener`

### Clean

```bash
make clean              # Clean library and objects
make clean-examples     # Clean example executables
make rebuild            # Clean and rebuild library
```

### All Targets

```bash
make                    # Default: build library
make all               # Same as default
make examples          # Build example programs
make clean             # Remove build artifacts
make clean-examples    # Remove example executables
make rebuild           # Clean and rebuild
```

## Using CMake

### Basic Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Output: `build/lib/libaukadicty.a`

### Build with Examples

```bash
mkdir build
cd build
cmake -DBUILD_EXAMPLES=ON ..
cmake --build .
```

Output:
- `build/lib/libaukadicty.a`
- `build/examples/example_basic`
- `build/examples/example_load`
- `build/examples/example_listener`

### Build Types

```bash
# Debug build (with symbols, no optimization)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (optimized, no symbols)
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Clean

```bash
# From build directory
rm -rf *

# Or rebuild from scratch
cd ..
rm -rf build
mkdir build && cd build
cmake .. && cmake --build .
```

### CMake Options

- `BUILD_EXAMPLES` - Build example programs (default: OFF)
- `CMAKE_BUILD_TYPE` - Debug or Release (default: unspecified)
- `CMAKE_C_COMPILER` - Specify C compiler
- `CMAKE_INSTALL_PREFIX` - Installation directory

Example with all options:

```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=ON \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  ..
```

## Cross-Compilation

### Using Make

Set compiler and flags:

```bash
CC=m68k-amigaos-gcc \
CFLAGS="-Wall -Wextra -std=c99 -O2 -I./include -I./os-include -I./cjson" \
make
```

### Using CMake

Create a toolchain file `amiga-toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME AmigaOS)
set(CMAKE_C_COMPILER m68k-amigaos-gcc)
set(CMAKE_FIND_ROOT_PATH /opt/amiga)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

Then build:

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=amiga-toolchain.cmake ..
cmake --build .
```

## Installation (CMake only)

```bash
cmake --install .
```

This installs:
- Library: `<prefix>/lib/libaukadicty.a`
- Headers: `<prefix>/include/aukadicty/*.h`

## IDE Integration

### Visual Studio Code

Use CMake Tools extension:
1. Install CMake Tools extension
2. Open project folder
3. Select kit (compiler)
4. Build with `Ctrl+Shift+B`

### CLion

CLion uses CMakeLists.txt automatically:
1. Open project
2. Build → Build Project

### Visual Studio (Windows)

1. File → Open → CMake
2. Select `CMakeLists.txt`
3. Build → Build All

## Output Structure

### Make Build

```
auka/
├── lib/
│   └── libaukadicty.a
├── src/*.o
├── cjson/*.o
└── examples/
    ├── example_basic
    ├── example_load
    └── example_listener
```

### CMake Build

```
auka/
└── build/
    ├── lib/
    │   └── libaukadicty.a
    ├── examples/           (if BUILD_EXAMPLES=ON)
    │   ├── example_basic
    │   ├── example_load
    │   └── example_listener
    └── CMakeFiles/         (build metadata)
```

## Using the Library

### With Make

```bash
gcc myapp.c -o myapp -I./include -L./lib -laukadicty
```

### With CMake (in your project)

```cmake
find_library(AUKADICTY_LIB aukadicty HINTS path/to/auka/lib)
include_directories(path/to/auka/include)
target_link_libraries(myapp ${AUKADICTY_LIB})
```

## Troubleshooting

### "AllocVec not found"

Ensure AmigaOS headers are in include path:
- Make: Included automatically via `-I./os-include`
- CMake: Included automatically via `include_directories()`

### Examples fail to link

Make sure library is built first:
```bash
make          # Build library first
make examples # Then build examples
```

### CMake can't find compiler

Specify explicitly:
```bash
cmake -DCMAKE_C_COMPILER=/path/to/gcc ..
```

## Performance Tips

### Parallel Builds (Make)

```bash
make -j4          # Use 4 parallel jobs
make -j$(nproc)   # Use all CPU cores
```

### Parallel Builds (CMake)

```bash
cmake --build . -j4
cmake --build . --parallel $(nproc)
```

## See Also

- [README.md](README.md) - Project overview and API documentation
- [CLAUDE.md](CLAUDE.md) - Development guidelines
- [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) - Project organization

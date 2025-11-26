# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Aukadicty is a sound mixing application for AmigaOS 3.1 written in C99. The project uses:
- AmigaOS 3.1 API exclusively (no POSIX, no modern C libraries)
- cJSON library for serialization (custom Amiga fork without float/double support)
- Document/View architecture pattern (Document layer must not depend on GUI)

## Critical Constraints

### Memory Management
- Use `AllocVec()` and `FreeVec()` from [os-include/clib/exec_protos.h](os-include/clib/exec_protos.h) for ALL allocations
- NO malloc/free/calloc/realloc
- Custom string management library required (no string class, manual allocation)

### File Operations
- Use DOS library functions from [os-include/clib/dos_protos.h](os-include/clib/dos_protos.h)
- All file paths must be stored relative to main project path

### Data Types
- NO float or double types allowed
- Use 64-bit fixed-point arithmetic for timing and real numbers
- Standard C functions limited to: memcpy(), memset(), strcpy(), strstr(), strcat() from <stdlib.h> and <string.h>

### Naming Convention
- All files and struct names start with "auk" prefix

## Architecture

### Object System (C99 OOP Simulation)

**Inheritance via struct composition:**
```c
// Base struct
struct BaseObject {
    void *(*New)(void);
    void (*Delete)(void *This);
    // ... other function pointers
};

// Derived struct - first member is parent type
struct DerivedObject {
    struct BaseObject base;  // MUST be first member
    // ... additional members
};
```

**Virtual methods via function pointers:**
- All method function pointers take `void *This` as first parameter
- Root object struct has `New` and `Delete` as first function pointers
- Each struct adds methods according to needs

### Shared Pointer System
- Custom C99 equivalent of std::shared_ptr
- Reference counting for automatic cleanup
- Object deleted after last release by calling Delete() then FreeVec()
- Enables directed graph of objects with shared ownership

### Document/View Pattern with Listener System

**Document layer**: Project data management (NO GUI dependencies allowed)
**View layer**: GUI representation

**Listener/Observer Pattern:**
- Every AukObject has built-in listener support
- GUI components (View) register as listeners on data objects (Document)
- When data is modified via Set functions, `SendUpdate()` is automatically called
- All registered listeners receive update notifications via callbacks
- Listeners are reference-counted via AukShared to prevent dangling pointers

**Usage:**
```c
/* Add listener to object */
object->base.AddListener(object, listenerShared, callbackFunction);

/* Remove listener */
object->base.RemoveListener(object, listenerPtr);

/* Automatic update on modification */
project->SetName(project, "New Name"); /* Triggers SendUpdate() */
```

**Important:** Set functions check if values actually changed before sending updates. If a value is set to the same value it already has, NO update is sent

### Data Model

The project is a directed graph of objects:

1. **Project** (root object)
   - Name and preferences
   - Audio mixing rate
   - Maximum track count (from prefs)
   - Pointer to track list

2. **Track** (in Gantt-diagram-like structure)
   - Time schedule
   - List of sounds attached to track
   - Sound envelope (B-spline interpolation using integers/fixed-point only)

3. **Sound** (abstract playable)
   - Start date and end date
   - Optional loop count
   - Can reference partial section of source audio
   - Points to shared SoundFile object

4. **SoundFile** (shared resource)
   - Filename (relative path)
   - Reference counted via shared pointer system
   - Multiple Sound objects can point to same SoundFile

All graph objects maintain pointer back to root Project.

### Serialization
- Save/load entire object graph to JSON using cJSON library
- cJSON is Amiga-forked version in [cjson/](cjson/) directory
- Note: `JSON_DONTUSE_FLOAT` is defined in [cjson/cJSON.h:33](cjson/cJSON.h#L33)

## Build Commands

### Using Make
```bash
make          # Build library
make clean    # Clean build artifacts
make rebuild  # Clean and rebuild
make examples # Build example programs (optional)
```

Builds: `lib/libaukadicty.a`

### Using CMake
```bash
mkdir build && cd build
cmake ..                              # Configure
cmake --build .                       # Build library

# Build with examples
cmake -DBUILD_EXAMPLES=ON ..
cmake --build .                       # Build library and examples
```

Builds: `build/lib/libaukadicty.a`
Examples (if enabled): `build/examples/example_*`

## Implemented Functionality

Core data management library (static library):
- Object system with inheritance and virtual methods
- Shared pointer reference counting
- String management (AllocVec-based)
- Fixed-point 64-bit arithmetic (32.32 format)
- Project/Track/Sound/SoundFile objects
- High-level operations (add/remove/move sounds, modify timing)
- JSON serialization/deserialization
- Track envelope management with B-spline interpolation

NOT yet implemented:
- Sound mixing engine
- WAV file loading/decoding
- Real-time playback
- GUI layer (View component)

## AmigaOS API Reference

Key documentation files:
- [os-include/dos_doc.txt](os-include/dos_doc.txt) - DOS library functions
- [os-include/exec_doc.txt](os-include/exec_doc.txt) - Exec library functions (memory, tasks, etc.)

Include paths:
- `os-include/clib/` - Function prototypes
- `os-include/dos/` - DOS structures and constants
- `os-include/exec/` - Exec structures and constants
- `os-include/proto/` - Pragma definitions

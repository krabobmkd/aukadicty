# AmigaStack - AmigaOS Compatibility Layer

AmigaStack is a lightweight compatibility layer that allows AmigaOS-specific code to compile and run on Linux and Windows platforms. It redirects AmigaOS system calls to their standard C equivalents.

## Purpose

This library enables cross-platform development of the Aukadicty project by providing implementations of AmigaOS exec.library and dos.library functions using standard C libraries.

## Supported Functions

### exec.library (proto/exec.h)
- **`AllocVec()`** → `calloc()` / `malloc()`
- **`FreeVec()`** → `free()`
- **`CreateMsgPort()`** → Simplified message port implementation
- **`DeleteMsgPort()`** → Free message port
- **`CreateNewProc()`** → `pthread_create()` (POSIX) / `_beginthreadex()` (Windows)
- Message passing functions (simplified stubs)

### dos.library (proto/dos.h)
- **`Open()`** → `fopen()`
- **`Read()`** → `fread()`
- **`Write()`** → `fwrite()`
- **`Close()`** → `fclose()`
- **`Seek()`** → `fseek()`
- **`DateStamp()`** → `time()` / `localtime()` (with Amiga epoch conversion)

## Directory Structure

```
AmigaStack/
├── include/
│   ├── proto/
│   │   ├── exec.h          # Exec function prototypes
│   │   └── dos.h           # DOS function prototypes
│   ├── exec/
│   │   ├── memory.h        # Memory flags
│   │   └── tasks.h         # Task/process structures
│   └── dos/
│       ├── dos.h           # DOS types (BPTR, modes)
│       └── dostags.h       # Process creation tags
├── src/
│   ├── exec.c              # Exec library implementation
│   └── dos.c               # DOS library implementation
└── README.md
```

## Build Integration

### CMake

AmigaStack is automatically included when building on non-Amiga platforms:

```bash
cmake -B build
cmake --build build
```

The CMakeLists.txt detects the platform:
- **AmigaOS**: Uses native `os-include/` headers
- **Linux/Windows**: Uses `AmigaStack/include/` headers and links compatibility layer

### Platform Detection

```cmake
if(NOT CMAKE_SYSTEM_NAME MATCHES "Amiga")
    set(USE_AMIGA_STACK ON)
    # Includes AmigaStack headers and sources
endif()
```

## Implementation Notes

### Memory Management
- `AllocVec()` with `MEMF_CLEAR` flag uses `calloc()` for zero-initialized memory
- `AllocVec()` without flags uses `malloc()`
- All allocations are standard heap allocations

### File I/O
- File handles (`BPTR`) are type-aliased to `FILE*`
- All file operations use binary mode (`"rb"`, `"wb"`)
- Seek modes are mapped to stdio equivalents

### Threading
- `CreateNewProc()` creates detached threads
- **POSIX**: Uses `pthread_create()` with `PTHREAD_CREATE_DETACHED`
- **Windows**: Uses `_beginthreadex()` with `CloseHandle()`

### Date/Time
- `DateStamp()` converts Unix epoch to Amiga epoch (1978-01-01)
- Time resolution: days, minutes, and ticks (1/50 second)

### Message Passing
- Message ports and inter-process communication are simplified
- Stub implementations for compatibility (aukstreamcache doesn't rely on full IPC)

## Limitations

1. **Message System**: Simplified stubs - full AmigaOS message passing not implemented
2. **Process Priority**: Priority and stack size tags are ignored in `CreateNewProc()`
3. **Signal System**: AmigaOS signals are not emulated
4. **DOS Packets**: Not implemented (not needed for current project)

## Compatibility

- ✅ **Linux** (tested with GCC, requires pthread)
- ✅ **Windows** (tested with MSVC and MinGW)
- ✅ **macOS** (should work with clang)
- ⚠️ **AmigaOS** (not used - native headers preferred)

## Future Extensions

If additional AmigaOS functions are needed:
1. Add prototypes to appropriate header in `include/proto/`
2. Implement in `src/exec.c` or `src/dos.c`
3. Update this README

## Example Usage

```c
#include <proto/exec.h>
#include <proto/dos.h>

/* Memory allocation */
void* buffer = AllocVec(1024, MEMF_CLEAR);
FreeVec(buffer);

/* File I/O */
BPTR file = Open("data.bin", MODE_OLDFILE);
if (file) {
    char buffer[256];
    long bytesRead = Read(file, buffer, 256);
    Close(file);
}

/* Date/time */
struct DateStamp ds;
DateStamp(&ds);
```

## License

Part of the Aukadicty project. See main project LICENSE for details.

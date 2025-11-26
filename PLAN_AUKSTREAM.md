# Plan: AukStreamCache - Sound Data Loading Engine

## Overview

Creating a separate sound data loading and caching library that runs in its own AmigaOS process, managing memory-constrained audio data for real-time playback.

## Requirements Summary

### Core Features
1. **Separate Process**: Runs independently from main application
2. **Memory Management**: Configurable cache (minimum 256KB), pre-allocated pools
3. **Audio Formats**: Support 8/16/32-bit signed, mono/stereo
4. **Conversion Modes**: 3 quality levels for memory/quality tradeoff
5. **File Formats**: .wave and IFF 8SVX (extensible for future formats)
6. **API**: Non-blocking preparation, synchronous data access
7. **Progress Tracking**: Real-time job counters
8. **Integration**: Depends on aukadicty lib, temporary AukObject retention

## Critical Questions for User

Before detailed implementation planning:

### 1. Process Communication Architecture
**Question**: How should the separate process communicate with the main application?
- **Option A**: AmigaOS message ports (exec.library Messages)
- **Option B**: Shared memory region
- **Option C**: Simple signals for synchronization
- **My Recommendation**: Message ports for structured communication

### 2. Multi-Process Model
**Question**: What's the preferred process architecture?
- **Option A**: Separate executable spawned by main app
- **Option B**: Shared library that creates its own process via CreateNewProc()
- **Option C**: Integrated but async using dos.library process functions
- **My Recommendation**: Option B - shared library with internal process creation

### 3. Cache Eviction Strategy
**Question**: When cache is full and new data needed, what should be discarded?
- **Option A**: LRU (Least Recently Used)
- **Option B**: Time-based (furthest from playback position)
- **Option C**: Priority-based (track importance)
- **My Recommendation**: Option B - time-based, most relevant for audio playback

### 4. PrepareData Blocking Behavior
**Question**: Should `aukstream_PrepareData()` block until:
- **Option A**: All requested audio is loaded
- **Option B**: Cache budget exhausted (partial load OK)
- **Option C**: Support both modes via parameter
- **My Recommendation**: Option A - complete load ensures predictable behavior

### 5. Thread Safety for StreamCacheResult
**Question**: How to protect shared counters (totalJobsToDo, totalJobsDone)?
- **Option A**: Use exec.library Semaphores
- **Option B**: Simple volatile access (atomic on 68k)
- **Option C**: No protection (read-only from other side)
- **My Recommendation**: Option B - volatile, 68k has atomic 32-bit ops

### 6. Implementation Priority
**Question**: File format implementation order?
- **Option A**: .wave first, then IFF 8SVX
- **Option B**: Both simultaneously
- **My Recommendation**: Option A - .wave first (more common)

## Preliminary Architecture

### Directory Structure
```
aukstreamcache/
├── include/
│   ├── aukstream.h          # Public API
│   ├── aukstreamtypes.h     # Data types and enums
│   ├── aukstreamcache.h     # Cache management (internal)
│   ├── aukstreamwave.h      # .wave reader (internal)
│   └── aukstream8svx.h      # IFF 8SVX reader (internal)
├── src/
│   ├── aukstream.c          # Main API & process management
│   ├── aukstreamproc.c      # Loader process implementation
│   ├── aukstreamcache.c     # Memory cache management
│   ├── aukstreamwave.c      # .wave file loader
│   ├── aukstream8svx.c      # IFF 8SVX file loader
│   └── aukstreampool.c      # Pre-allocated memory pools
└── Makefile / CMakeLists.txt
```

### Key Components

#### 1. Public API (`aukstream.h`)
```c
typedef enum {
    AUK_STREAM_8BIT = 0,
    AUK_STREAM_16BIT = 1,
    AUK_STREAM_32BIT = 2
} AukStreamDataType;

typedef enum {
    AUK_STREAM_CONVERT_8BIT = 1,      /* Convert everything to 8-bit */
    AUK_STREAM_KEEP_16BIT = 2,        /* Keep 8/16, convert 32->16 */
    AUK_STREAM_KEEP_ALL = 3           /* Keep original formats */
} AukStreamConversionMode;

typedef struct {
    volatile unsigned long totalJobsToDo;
    volatile unsigned long totalJobsDone;
    volatile int isActive;              /* 1 if process is working */
    volatile int lastError;             /* Error code if failed */
} AukStreamCacheResult;

typedef struct AukStreamEngine AukStreamEngine;

/* Initialize engine with preferences */
AukStreamEngine* aukstream_Init(unsigned long maxCacheMemory,
                                AukStreamConversionMode conversionMode);

/* Shutdown and cleanup */
void aukstream_Shutdown(AukStreamEngine* engine);

/* Prepare data for playback (blocking in loader process) */
int aukstream_PrepareData(AukStreamEngine* engine,
                          AukProject* project,
                          AukFixed startTime,
                          AukFixed endTime,
                          AukStreamCacheResult* result);

/* Get audio data from cache (fast, may return NULL or partial) */
void* aukstream_GetAudioStream(AukStreamEngine* engine,
                               AukSoundFile* soundFile,
                               unsigned long frameStart,
                               unsigned long length,
                               AukStreamDataType* returnDataType,
                               unsigned long* effectiveStart,
                               unsigned long* effectiveEnd);

/* Check if data range is in cache */
int aukstream_IsCached(AukStreamEngine* engine,
                       AukSoundFile* soundFile,
                       unsigned long frameStart,
                       unsigned long length);

/* Clear cache */
void aukstream_ClearCache(AukStreamEngine* engine);
```

#### 2. Cache Management
- Pre-allocated memory pools (avoid runtime AllocVec)
- Chunk-based storage (e.g., 64KB chunks)
- Time-based eviction (discard data far from playback point)
- Track which AukSoundFile regions are cached

#### 3. File Format Loaders
- Modular design for adding new formats
- Common interface for all readers
- Support partial file loading (specific frame ranges)

#### 4. Process Model
- Main app creates AukStreamEngine
- Engine spawns loader process via CreateNewProc()
- Communication via message ports
- Loader process reads project graph, loads files
- Main app polls StreamCacheResult for progress

### Memory Management Strategy

#### Pre-Allocation Approach
1. **Startup**: Allocate large pool(s) totaling maxCacheMemory
2. **Chunk System**: Divide into fixed-size chunks (64KB each)
3. **Sub-Allocation**: Manage chunks internally, no runtime AllocVec
4. **Benefit**: Predictable, no freezing during playback

#### Cache Structure
```c
typedef struct AukStreamChunk {
    void* data;                    /* Pre-allocated memory */
    AukSoundFile* soundFile;       /* Which file this belongs to */
    unsigned long frameStart;      /* Start frame in file */
    unsigned long frameEnd;        /* End frame in file */
    AukStreamDataType dataType;    /* Format of stored data */
    AukFixed lastAccessTime;       /* For LRU/time-based eviction */
    struct AukStreamChunk* next;   /* Linked list */
} AukStreamChunk;

typedef struct {
    AukStreamChunk* chunks;        /* All available chunks */
    unsigned long chunkSize;       /* Size of each chunk (64KB) */
    unsigned long chunkCount;      /* Total chunks available */
    unsigned long usedChunks;      /* Currently in use */
} AukStreamCache;
```

### Conversion Pipeline

#### Mode 1: Convert All to 8-bit
- All audio converted to 8-bit signed on load
- Smallest memory footprint
- Lowest quality

#### Mode 2: Keep 8/16-bit, Convert 32->16
- 8-bit files stay 8-bit
- 16-bit files stay 16-bit
- 32-bit files converted to 16-bit
- Balance of quality and memory

#### Mode 3: Keep All Formats
- No conversion, store in original format
- Highest quality
- Largest memory usage

### File Format Support

#### .wave (RIFF WAVE)
- Read RIFF header
- Parse fmt chunk (sample rate, bit depth, channels)
- Load data chunk
- Support 8/16/24/32-bit PCM

#### IFF 8SVX (Amiga native)
- Read FORM header
- Parse VHDR (voice header)
- Load BODY chunk
- Support 8-bit signed/unsigned

### Integration with Aukadicty

#### Reading Project Graph
```c
/* In loader process */
void LoaderProcess() {
    /* Retain project while reading */
    AukShared* projectShared = GetProjectShared();
    AukProject* project = AukShared_GetObject(projectShared);

    /* Read tracks, sounds, sound files */
    for each track in project {
        for each sound in track {
            if (sound in time range) {
                AukShared* fileShared = sound->soundFile;
                AukSoundFile* file = AukShared_GetObject(fileShared);

                /* Load from disk */
                LoadAudioFile(file->filename, ...);

                /* Release ASAP after reading */
            }
        }
    }

    AukShared_Release(projectShared);
}
```

### Process Communication

#### Message Port Design
```c
/* Messages sent to loader process */
typedef struct {
    struct Message msg;
    enum {
        AUK_STREAM_MSG_PREPARE,
        AUK_STREAM_MSG_CANCEL,
        AUK_STREAM_MSG_SHUTDOWN
    } type;

    /* For PREPARE messages */
    AukProject* project;
    AukFixed startTime;
    AukFixed endTime;
    AukStreamCacheResult* result;
} AukStreamMessage;
```

## Implementation Phases

### Phase 1: Core Infrastructure
1. Basic engine initialization
2. Memory pool allocation system
3. Chunk management
4. Process creation/shutdown

### Phase 2: Cache Management
1. Chunk allocation/deallocation
2. Eviction strategy
3. Cache lookup/insertion
4. Statistics tracking

### Phase 3: File Format - .wave
1. RIFF parser
2. fmt chunk reader
3. data chunk loader
4. Conversion pipeline
5. Partial file loading

### Phase 4: File Format - IFF 8SVX
1. IFF parser
2. VHDR reader
3. BODY loader
4. Format conversion

### Phase 5: Process & API
1. CreateNewProc() setup
2. Message port communication
3. PrepareData implementation
4. GetAudioStream implementation

### Phase 6: Integration Testing
1. Test with simple project
2. Test memory limits
3. Test all conversion modes
4. Performance profiling

## Dependencies

### AmigaOS Libraries
- exec.library: AllocVec, FreeVec, CreateNewProc, message ports
- dos.library: Open, Read, Seek, Close

### Internal Dependencies
- aukadicty library: AukProject, AukTrack, AukSound, AukSoundFile
- aukadicty library: AukShared, AukFixed types

## Build System

### Makefile Target
```makefile
STREAMCACHE_SRC = aukstreamcache/src/*.c
STREAMCACHE_LIB = lib/libaukstreamcache.a

$(STREAMCACHE_LIB): $(STREAMCACHE_SRC)
    # Build static library
```

### CMake Target
```cmake
add_library(aukstreamcache STATIC
    aukstreamcache/src/aukstream.c
    aukstreamcache/src/aukstreamproc.c
    # ...
)
target_link_libraries(aukstreamcache aukadicty)
```

## Next Steps After Clarifications

Once user answers questions above:
1. Finalize process communication mechanism
2. Detail cache eviction algorithm
3. Specify PrepareData blocking semantics
4. Begin Phase 1 implementation

## Risk Mitigation

### Memory Fragmentation
- **Solution**: Pre-allocation eliminates runtime fragmentation

### Process Overhead
- **Solution**: Single persistent process, not spawned per operation

### File I/O Blocking
- **Solution**: All I/O in separate process, won't block main app

### Cache Misses
- **Solution**: PrepareData called ahead of playback, predictive loading

## Decisions Made

### User Confirmed
- **Zero-copy API**: `aukstream_GetAudioStream()` returns `const void*` pointer to cache data

### Recommended Approach (proceeding with)
1. **Process Model**: Library manages internal process, main app calls blocking functions
2. **Cache Eviction**: Time-based (discard audio furthest from requested timeslice)
3. **PrepareData Blocking**: Load all audio for timeslice before returning
4. **Thread Safety**: Volatile counters (68k has atomic 32-bit operations)
5. **Implementation Priority**: .wave first, then IFF 8SVX
6. **Memory Strategy**: Multiple fixed-size chunks (16KB each)

## Detailed Implementation Plan

### API Design (Final)

```c
/* Return from GetAudioStream - const pointer, zero-copy */
const void* aukstream_GetAudioStream(
    AukStreamEngine* engine,
    AukSoundFile* soundFile,
    unsigned long frameStart,
    unsigned long length,
    AukStreamDataType* returnDataType,
    unsigned long* effectiveStart,
    unsigned long* effectiveEnd
);
```

### Architecture Components

#### 1. Engine Structure
```c
struct AukStreamEngine {
    /* Configuration */
    unsigned long maxCacheMemory;        /* e.g., 262144 for 256KB */
    AukStreamConversionMode conversionMode;

    /* Memory pool */
    void* memoryPool;                    /* Large pre-allocated buffer */
    AukStreamCache* cache;               /* Cache management */

    /* Process management */
    struct Process* loaderProcess;       /* Created via CreateNewProc */
    struct MsgPort* commandPort;         /* For sending commands */
    struct MsgPort* replyPort;           /* For receiving replies */

    /* State */
    int isActive;                        /* Is loader process running? */
    AukStreamCacheResult* currentResult; /* Current operation status */
};
```

#### 2. Cache Chunk System
```c
#define AUK_STREAM_CHUNK_SIZE (16 * 1024)  /* 16KB chunks */

typedef struct AukStreamChunk {
    const void* data;                   /* Points into memory pool */
    AukSoundFile* soundFile;            /* Which file (weak ref) */
    unsigned long frameStart;
    unsigned long frameCount;
    AukStreamDataType dataType;
    unsigned channels;
    AukFixed lastAccessTime;            /* For time-based eviction */
    int isLocked;                       /* Don't evict if locked */
    struct AukStreamChunk* next;
} AukStreamChunk;
```

#### 3. Loader Process Communication
```c
/* Message types sent to loader process */
typedef struct AukStreamCommand {
    struct Message msg;
    enum {
        CMD_PREPARE,      /* Load audio for timeslice */
        CMD_CLEAR,        /* Clear cache */
        CMD_SHUTDOWN      /* Terminate process */
    } type;

    /* For CMD_PREPARE */
    AukShared* projectShared;  /* Retained project reference */
    AukFixed startTime;
    AukFixed endTime;
    AukStreamCacheResult* result;  /* Volatile counters updated here */
} AukStreamCommand;
```

### Implementation Phases (Detailed)

#### Phase 1: Core Infrastructure (Days 1-2)
Files to create:
- `aukstreamcache/include/aukstream.h` - Public API
- `aukstreamcache/include/aukstreamtypes.h` - Enums and types
- `aukstreamcache/src/aukstream.c` - Engine init/shutdown
- `aukstreamcache/src/aukstreampool.c` - Memory pool management

Tasks:
1. Engine initialization
   - Allocate memory pool (one large AllocVec)
   - Divide into fixed chunks (16KB each)
   - Create free chunk list
2. Chunk allocation/free (from pool, no AllocVec)
3. Basic cache structure
4. Engine shutdown and cleanup

#### Phase 2: Process Management (Days 3-4)
Files to create:
- `aukstreamcache/src/aukstreamproc.c` - Loader process
- `aukstreamcache/src/aukstreammsg.c` - Message handling

Tasks:
1. CreateNewProc() wrapper
2. Message port setup
3. Command message sending
4. Loader process main loop
5. Process termination handling

#### Phase 3: Cache Management (Days 5-6)
Files to create:
- `aukstreamcache/src/aukstreamcache.c` - Cache operations

Tasks:
1. Cache lookup (by soundFile + frame range)
2. Cache insertion (allocate chunks from pool)
3. Time-based eviction algorithm
4. Lock/unlock mechanisms
5. Cache statistics

#### Phase 4: .wave File Loader (Days 7-9)
Files to create:
- `aukstreamcache/include/aukstreamwave.h` - Internal API
- `aukstreamcache/src/aukstreamwave.c` - RIFF WAVE reader

Tasks:
1. RIFF header parser
2. fmt chunk reader (PCM format detection)
3. data chunk locator
4. Partial file loading (seek to frame range)
5. Format conversion pipeline:
   - 8-bit unsigned → 8-bit signed
   - 16-bit → keep or convert
   - 24/32-bit → 16-bit or keep (based on mode)
   - Stereo ↔ mono conversions

#### Phase 5: IFF 8SVX Loader (Days 10-11)
Files to create:
- `aukstreamcache/include/aukstream8svx.h` - Internal API
- `aukstreamcache/src/aukstream8svx.c` - IFF 8SVX reader

Tasks:
1. IFF FORM parser
2. VHDR chunk reader
3. BODY chunk loader
4. 8-bit unsigned/signed handling
5. Compression support (if needed)

#### Phase 6: PrepareData Implementation (Days 12-13)
Tasks:
1. Walk project graph (tracks → sounds)
2. Filter sounds by time range
3. Build load job list
4. Update totalJobsToDo counter
5. Load files one by one:
   - Open file
   - Read needed frames
   - Convert format
   - Store in cache chunks
   - Update totalJobsDone
6. Handle partial files (fileStartFrame, fileEndFrame)
7. Handle loops (load extra if needed)

#### Phase 7: GetAudioStream Implementation (Day 14)
Tasks:
1. Lookup in cache
2. Return const pointer
3. Fill return parameters (dataType, effectiveStart/End)
4. Handle partial hits
5. Lock chunks to prevent eviction during playback

#### Phase 8: Testing & Integration (Days 15-16)
Tasks:
1. Unit tests for each component
2. Integration test with aukadicty project
3. Memory leak detection
4. Performance profiling
5. Edge case testing

### Build System Integration

#### Makefile
```makefile
# Add to existing Makefile

STREAMCACHE_SRC = aukstreamcache/src/aukstream.c \
                  aukstreamcache/src/aukstreampool.c \
                  aukstreamcache/src/aukstreamproc.c \
                  aukstreamcache/src/aukstreammsg.c \
                  aukstreamcache/src/aukstreamcache.c \
                  aukstreamcache/src/aukstreamwave.c \
                  aukstreamcache/src/aukstream8svx.c

STREAMCACHE_OBJ = $(STREAMCACHE_SRC:.c=.o)
STREAMCACHE_LIB = lib/libaukstreamcache.a

$(STREAMCACHE_LIB): $(STREAMCACHE_OBJ)
	@mkdir -p lib
	$(AR) $(ARFLAGS) $@ $^

streamcache: $(STREAMCACHE_LIB)
```

#### CMakeLists.txt
```cmake
# Add new library target

set(AUKSTREAMCACHE_SOURCES
    aukstreamcache/src/aukstream.c
    aukstreamcache/src/aukstreampool.c
    aukstreamcache/src/aukstreamproc.c
    aukstreamcache/src/aukstreammsg.c
    aukstreamcache/src/aukstreamcache.c
    aukstreamcache/src/aukstreamwave.c
    aukstreamcache/src/aukstream8svx.c
)

add_library(aukstreamcache STATIC ${AUKSTREAMCACHE_SOURCES})
target_include_directories(aukstreamcache PUBLIC
    aukstreamcache/include
    include
    os-include
)
target_link_libraries(aukstreamcache aukadicty)
```

### Memory Budget Example (256KB minimum)

```
Total: 256KB (262,144 bytes)
Chunk size: 16KB (16,384 bytes)
Number of chunks: 16

Per chunk overhead: ~64 bytes (AukStreamChunk struct)
Total overhead: 1KB
Usable data: 255KB

Example distribution:
- 8-bit mono @ 44.1kHz: ~5.8 seconds of audio
- 16-bit stereo @ 44.1kHz: ~1.45 seconds of audio
- Can mix formats in cache
```

### Critical Implementation Details

#### Time-Based Eviction Algorithm
```c
void EvictFurthestChunk(AukStreamCache* cache, AukFixed targetTime) {
    AukStreamChunk* furthest = NULL;
    AukFixed maxDistance = 0;

    for each chunk in cache {
        if (chunk->isLocked) continue;

        AukFixed distance = AukFixed_Abs(
            AukFixed_Sub(chunk->lastAccessTime, targetTime)
        );

        if (distance > maxDistance) {
            maxDistance = distance;
            furthest = chunk;
        }
    }

    if (furthest) {
        FreeChunk(furthest);  /* Back to free list, no FreeVec */
    }
}
```

#### Zero-Copy Access
```c
const void* aukstream_GetAudioStream(...) {
    AukStreamChunk* chunk = FindChunk(soundFile, frameStart);

    if (!chunk) return NULL;

    chunk->isLocked = 1;  /* Prevent eviction during playback */
    chunk->lastAccessTime = currentTime;

    *returnDataType = chunk->dataType;
    *effectiveStart = chunk->frameStart;
    *effectiveEnd = chunk->frameStart + chunk->frameCount;

    return chunk->data;  /* Direct pointer into memory pool */
}
```

#### Unlock API (needed)
```c
/* Add to public API */
void aukstream_ReleaseAudioStream(AukStreamEngine* engine, const void* data);
```

### Next Steps

Ready to begin implementation with:
1. Phase 1 (Core Infrastructure) first
2. Zero-copy API with const pointers
3. Time-based cache eviction
4. .wave format priority
5. 16KB chunk size, pre-allocated pool

Proceeding with implementation plan.

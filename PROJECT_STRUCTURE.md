# Aukadicty Project Structure

## Directory Layout

```
auka/
├── CLAUDE.md              # Claude Code guidance
├── README.md              # Project documentation
├── PROJECT_STRUCTURE.md   # This file
├── Makefile               # GNU Make build system
├── CMakeLists.txt         # CMake build system
├── .gitignore             # Git ignore patterns
│
├── include/               # Public header files
│   ├── aukadicty.h        # Main include (includes all below)
│   ├── aukobject.h        # Base object system
│   ├── aukshared.h        # Shared pointer system
│   ├── aukstring.h        # String utilities
│   ├── aukfixed.h         # Fixed-point arithmetic
│   ├── aukproject.h       # Project object
│   ├── auktrack.h         # Track object
│   ├── auksound.h         # Sound object
│   ├── auksoundfile.h     # Sound file object
│   ├── aukoperations.h    # High-level operations
│   └── aukjson.h          # JSON serialization
│
├── src/                   # Implementation files
│   ├── aukobject.c        # Base object implementation
│   ├── aukshared.c        # Shared pointer implementation
│   ├── aukstring.c        # String utilities implementation
│   ├── aukfixed.c         # Fixed-point math implementation
│   ├── aukproject.c       # Project implementation
│   ├── auktrack.c         # Track implementation
│   ├── auksound.c         # Sound implementation
│   ├── auksoundfile.c     # Sound file implementation
│   ├── aukoperations.c    # High-level operations implementation
│   └── aukjson.c          # JSON serialization implementation
│
├── cjson/                 # cJSON library (Amiga fork)
│   ├── cJSON.h            # cJSON header
│   └── cJSON.c            # cJSON implementation
│
├── os-include/            # AmigaOS 3.1 headers
│   ├── clib/              # C library prototypes
│   │   ├── exec_protos.h  # Exec library (AllocVec, FreeVec, etc.)
│   │   └── dos_protos.h   # DOS library (Open, Read, Write, etc.)
│   ├── exec/              # Exec structures and constants
│   ├── dos/               # DOS structures and constants
│   ├── proto/             # Pragma definitions
│   ├── exec_doc.txt       # Exec library documentation
│   └── dos_doc.txt        # DOS library documentation
│
├── examples/              # Example programs
│   ├── example_basic.c    # Basic usage example
│   └── example_load.c     # Load project example
│
├── lib/                   # Build output (generated)
│   └── libaukadicty.a     # Static library
│
└── build/                 # CMake build directory (generated)
```

## Module Overview

### Core System (aukobject.h/c)
- Base object structure with New/Delete/GetTypeName methods
- Foundation for object-oriented design in C99
- All objects inherit from AukObject

### Shared Pointers (aukshared.h/c)
- Reference-counted smart pointers
- Automatic memory cleanup when refcount reaches zero
- Enables shared ownership of objects (like SoundFile)

### String Utilities (aukstring.h/c)
- String operations using AllocVec/FreeVec
- Duplicate, concatenate, compare, find operations
- Path manipulation (relative/absolute conversion)

### Fixed-Point Math (aukfixed.h/c)
- 64-bit fixed-point arithmetic (32.32 format)
- Conversion, arithmetic, comparison operations
- Linear interpolation (lerp) for envelopes

### Data Model Objects

#### Project (aukproject.h/c)
- Root of the object graph
- Contains tracks, preferences, project metadata
- Save/Load methods for JSON serialization

#### Track (auktrack.h/c)
- Represents one track in the timeline
- Contains array of sounds
- Envelope system with B-spline interpolation

#### Sound (auksound.h/c)
- Instance of a sound on a track
- Timing information (start/end)
- File range and loop count
- References shared SoundFile

#### SoundFile (auksoundfile.h/c)
- Shared resource representing audio file
- Metadata: filename, sample rate, channels, frame count
- Reference counted via AukShared

### High-Level Operations (aukoperations.h/c)
- Convenience functions for common tasks
- Add/remove/move sounds
- Create tracks and sound files
- Query project duration
- Find sounds at specific times

### JSON Serialization (aukjson.h/c)
- Save/Load entire project graph
- Uses cJSON library
- Handles relative path conversion
- Preserves all object relationships

## Build Outputs

### Static Library
- `lib/libaukadicty.a` (Make) or `build/lib/libaukadicty.a` (CMake)
- Links against AmigaOS libraries: exec.library, dos.library

### Examples (Optional)
- `examples/example_basic` - Demonstrates creating projects
- `examples/example_load` - Demonstrates loading projects

## Dependencies

### AmigaOS 3.1 Libraries
- **exec.library**: Memory allocation (AllocVec/FreeVec)
- **dos.library**: File I/O (Open/Read/Write/Close)

### Third-Party
- **cJSON**: JSON parser (Amiga fork, no float/double)

### Standard C (Limited)
- memcpy, memset, strcpy, strstr, strcat from <string.h>
- No malloc/free, no fopen/fread/fwrite

## Object Relationships

```
AukProject
  │
  ├─── tracks[] ───> AukTrack
  │                    │
  │                    ├─── sounds[] ───> AukSound
  │                    │                     │
  │                    │                     └─── soundFile (AukShared)
  │                    │                            │
  │                    │                            └─> AukSoundFile
  │                    │
  │                    └─── envelope ───> AukEnvelopePoint
  │                                         │
  │                                         └─── next ───> AukEnvelopePoint
  │
  └─── preferences (AukProjectPrefs)
```

## Memory Ownership

- **Project** owns **Tracks** (deletes on project delete)
- **Track** owns **Sounds** (deletes on track delete)
- **Sound** shares **SoundFile** via AukShared (reference counted)
- **Track** owns **EnvelopePoints** (linked list, deleted on track delete)

## File Format

Projects are saved as JSON with the following structure:

```json
{
  "version": "1.0",
  "name": "Project Name",
  "preferences": {
    "sampleRate": 44100,
    "maxTracks": 64
  },
  "tracks": [
    {
      "name": "Track 1",
      "sounds": [
        {
          "file": "sounds/sample.wav",
          "sampleRate": 44100,
          "channels": 2,
          "frameCount": 88200,
          "startTime": 0,
          "endTime": 5000,
          "fileStartFrame": 0,
          "fileEndFrame": 0,
          "loopCount": 0
        }
      ],
      "envelope": [
        {"time": 0, "value": 1000},
        {"time": 5000, "value": 500}
      ]
    }
  ]
}
```

Note: Times and values are stored as integers (fixed-point scaled by 1000).

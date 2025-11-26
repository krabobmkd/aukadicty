# Aukadicty

A sound mixing application for AmigaOS 3.1 written in C99.

## Overview

Aukadicty is a multi-track audio project management library designed specifically for AmigaOS 3.1. It provides a pure C99 implementation with no dependencies on modern standard libraries, using only AmigaOS 3.1 API calls for memory management and file operations.

## Key Features

- **Pure C99 Implementation**: Compatible with AmigaOS 3.1
- **Object-Oriented Design**: Uses struct composition for inheritance and function pointers for virtual methods
- **Smart Pointer System**: Reference-counted shared pointers for automatic memory management
- **Fixed-Point Arithmetic**: No floating-point operations (32.32 fixed-point format)
- **Document/View Architecture**: Clean separation between data model and GUI
- **JSON Serialization**: Save and load complete project graphs using cJSON
- **Gantt-Style Timeline**: Multi-track timeline with sound scheduling
- **B-Spline Envelopes**: Track volume/pan control with interpolated envelope points

## Architecture

### Object System

All objects inherit from `AukObject` which provides:
- `New` - Constructor
- `Delete` - Destructor
- `GetTypeName` - Runtime type information

Inheritance is achieved by placing the parent struct as the first member:

```c
struct AukSound {
    AukObject base;  /* Must be first */
    /* ... additional members ... */
};
```

### Data Model

The project consists of a directed graph:

```
AukProject (root)
  ├─ AukTrack[]
  │   ├─ AukSound[]
  │   │   └─ AukShared<AukSoundFile>
  │   └─ AukEnvelopePoint (linked list)
  └─ Preferences
```

- **AukProject**: Root object containing tracks and project settings
- **AukTrack**: Represents a single track with sounds and envelope
- **AukSound**: Sound instance with timing and loop information
- **AukSoundFile**: Shared file resource (reference counted)
- **AukEnvelopePoint**: Volume/pan control points for interpolation

### Memory Management

- All allocations use `AllocVec()` from `exec.library`
- All deallocations use `FreeVec()`
- Shared resources use reference counting via `AukShared`
- Objects are automatically freed when last reference is released

### Fixed-Point Math

All real numbers use 64-bit fixed-point (32.32 format):

```c
AukFixed time = AukFixed_FromInt(5);           // 5 seconds
AukFixed half = AukFixed_FromFraction(1, 2);   // 0.5
AukFixed result = AukFixed_Mul(time, half);    // 2.5 seconds
```

## Building

### Using Make

```bash
make
```

This creates `lib/libaukadicty.a`

### Using CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This creates `build/lib/libaukadicty.a`

To build with examples:

```bash
cmake -DBUILD_EXAMPLES=ON ..
cmake --build .
```

This creates examples in `build/examples/`

### Clean Build

```bash
# Make
make clean
make rebuild

# CMake
rm -rf build
mkdir build && cd build && cmake .. && cmake --build .
```

## Usage

### Creating a Project

```c
#include <aukadicty.h>

/* Create project */
AukProject* project = AukOp_CreateProject("My Project", "Work:");
AukProject_SetPreferences(project, 44100, 16);

/* Add track */
AukTrack* track = AukOp_AddTrackToProject(project, "Track 1");

/* Create sound file */
AukShared* soundFile = AukOp_CreateSoundFile("sounds/test.wav", 44100, 2, 88200);

/* Add sound to track */
AukSound* sound = AukOp_AddSoundToTrack(track, soundFile,
                                        AukFixed_FromInt(0),   /* Start */
                                        AukFixed_FromInt(5));  /* End */

/* Save project */
project->Save(project, "Work:project.auk");

/* Clean up */
AukShared_Release(soundFile);
AukProject_Delete(project);
```

### Loading a Project

```c
/* Load project from file */
AukProject* project = AukJson_LoadProject("Work:project.auk");

/* Access tracks */
unsigned long trackCount = project->GetTrackCount(project);
AukTrack* track = project->GetTrack(project, 0);

/* Access sounds */
unsigned long soundCount = track->GetSoundCount(track);
AukSound* sound = track->GetSound(track, 0);

/* Clean up */
AukProject_Delete(project);
```

### Manipulating Sounds

```c
/* Move sound between tracks */
AukOp_MoveSoundToTrack(sound, newTrack);

/* Change timing */
AukOp_SetSoundTimeRange(sound, AukFixed_FromInt(2), AukFixed_FromInt(7));

/* Set loop count */
AukOp_SetSoundLoopCount(sound, 3);

/* Remove sound */
AukOp_RemoveSound(sound);
```

### Working with Envelopes

```c
/* Add envelope points (time, value) */
track->AddEnvelopePoint(track, AukFixed_FromInt(0), AukFixed_FromInt(1));
track->AddEnvelopePoint(track, AukFixed_FromInt(5), AukFixed_FromFraction(1, 2));

/* Get interpolated value at time */
AukFixed value = track->GetEnvelopeValue(track, AukFixed_FromInt(3));
```

### Listener Pattern (Document/View)

The library implements a built-in observer pattern for Document/View architecture:

```c
/* Define update callback */
void OnProjectUpdate(void* listenerObject, void* modifiedObject) {
    /* Update GUI when project changes */
    printf("Project was modified!\n");
}

/* Create listener (GUI widget) and wrap in shared pointer */
GUIWidget* widget = CreateWidget();
AukShared* widgetShared = AukShared_Create(widget, &widget->base);

/* Register listener on data object */
project->base.AddListener(project, widgetShared, OnProjectUpdate);

/* Modifications automatically trigger updates */
project->SetName(project, "New Name");  /* Calls OnProjectUpdate */

/* Unregister when done */
project->base.RemoveListener(project, widget);
AukShared_Release(widgetShared);
```

**Key Features:**
- All data modifications send update notifications
- Updates only sent when values actually change
- Listeners are reference-counted to prevent dangling pointers
- GUI layer can subscribe without Document layer knowing about GUI

## API Reference

### Core Objects

- `AukProject` - Root project container
- `AukTrack` - Track with sounds and envelope
- `AukSound` - Sound instance with timing
- `AukSoundFile` - Shared sound file resource

### Utilities

- `AukString_*` - String operations using AllocVec/FreeVec
- `AukFixed_*` - Fixed-point arithmetic (32.32 format)
- `AukShared_*` - Reference-counted smart pointers
- `AukOp_*` - High-level convenience operations
- `AukJson_*` - JSON serialization/deserialization

## File Structure

```
auka/
├── include/          # Public headers
│   ├── aukadicty.h   # Main include file
│   ├── aukobject.h   # Base object system
│   ├── aukshared.h   # Shared pointers
│   ├── aukstring.h   # String utilities
│   ├── aukfixed.h    # Fixed-point math
│   ├── aukproject.h  # Project object
│   ├── auktrack.h    # Track object
│   ├── auksound.h    # Sound object
│   ├── auksoundfile.h # Sound file object
│   ├── aukoperations.h # High-level operations
│   └── aukjson.h     # JSON serialization
├── src/              # Implementation files
├── cjson/            # cJSON library (Amiga fork)
├── os-include/       # AmigaOS 3.1 headers
├── examples/         # Example programs
├── lib/              # Build output
├── Makefile          # GNU Make build
└── CMakeLists.txt    # CMake build
```

## Examples

See the `examples/` directory for complete working examples:

- `example_basic.c` - Creating and saving a project
- `example_load.c` - Loading and inspecting a project
- `example_listener.c` - Using the listener/observer pattern for Document/View

## Constraints

### AmigaOS 3.1 API Only

- Memory: `AllocVec()` / `FreeVec()` from exec.library
- Files: `Open()` / `Read()` / `Write()` / `Close()` from dos.library
- NO malloc/free, NO fopen/fread/fwrite

### No Floating Point

- All real numbers use 64-bit fixed-point (32.32)
- Timing, interpolation, and calculations use integer arithmetic

### Limited Standard Library

- Allowed: `memcpy()`, `memset()`, `strcpy()`, `strstr()`, `strcat()`
- String management requires manual AllocVec/FreeVec

### Document Layer Independence

- Project data model has NO GUI dependencies
- Document layer must never include GUI code
- Communication via update messages only

## License

This project uses cJSON which is licensed under the MIT License.

## Future Work

The following components are planned but not yet implemented:

- Sound mixing engine
- WAV file loading/decoding
- Real-time playback
- GUI layer (View implementation)
- Export to audio file

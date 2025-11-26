# Listener/Observer Pattern Implementation

## Overview

The Aukadicty library implements a complete listener/observer pattern at the base object level, enabling clean Document/View architecture where GUI components can subscribe to data model changes without the data layer depending on the GUI.

## Architecture

### Base Object Support

Every `AukObject` includes built-in listener support:

```c
struct AukObject {
    /* ... other members ... */

    /* Listener management methods */
    int (*AddListener)(void* This, AukShared* listenerObject, AukUpdateCallback callback);
    int (*RemoveListener)(void* This, void* listenerObject);
    void (*SendUpdate)(void* This);

    /* Listener list */
    AukListener* listeners;
};
```

### Listener Node Structure

Listeners are stored in a linked list:

```c
struct AukListener {
    AukShared* listenerObject;      /* Reference-counted listener */
    AukUpdateCallback callback;     /* Notification function */
    AukListener* next;              /* Next in list */
};
```

### Update Callback

The callback function signature:

```c
typedef void (*AukUpdateCallback)(void* listenerObject, void* modifiedObject);
```

Parameters:
- `listenerObject` - The object that is listening (e.g., GUI widget)
- `modifiedObject` - The data object that was modified (e.g., Project, Track, Sound)

## Usage

### 1. Create Listener Object

The listener can be any object (typically a GUI component):

```c
typedef struct MyGUIWidget {
    AukObject base;  /* Inherit from AukObject */
    /* ... widget-specific data ... */
} MyGUIWidget;
```

### 2. Implement Update Callback

```c
void MyWidget_OnDataUpdate(void* listenerObject, void* modifiedObject) {
    MyGUIWidget* widget = (MyGUIWidget*)listenerObject;
    AukObject* dataObj = (AukObject*)modifiedObject;

    /* Refresh widget display based on data change */
    printf("Data object %s was modified\n", dataObj->GetTypeName(dataObj));

    /* Update widget visuals, labels, etc. */
    widget->Refresh(widget);
}
```

### 3. Register Listener

Listeners must be wrapped in `AukShared` for reference counting:

```c
/* Create GUI widget */
MyGUIWidget* widget = MyWidget_New();

/* Wrap in shared pointer */
AukShared* widgetShared = AukShared_Create(widget, &widget->base);

/* Register as listener on data object */
project->base.AddListener(project, widgetShared, MyWidget_OnDataUpdate);
```

### 4. Automatic Notifications

All Set functions automatically send updates when data changes:

```c
/* This triggers SendUpdate() which calls all registered callbacks */
project->SetName(project, "New Project Name");

/* This also triggers update */
track->AddSound(track, sound);

/* NO update sent if value doesn't change */
project->SetName(project, "New Project Name");  /* Same value - no update */
```

### 5. Unregister Listener

```c
/* Remove listener before cleanup */
project->base.RemoveListener(project, widget);

/* Release shared reference */
AukShared_Release(widgetShared);

/* Delete widget */
MyWidget_Delete(widget);
```

## Memory Management

### Reference Counting

- Listeners are stored as `AukShared*` pointers
- When added: `AukShared_Retain()` is called automatically
- When removed: `AukShared_Release()` is called automatically
- Prevents dangling pointers if listener is deleted while still registered

### Automatic Cleanup

When an object is deleted, all listeners are automatically released:

```c
void AukObject_Delete(void* This) {
    AukObject* obj = (AukObject*)This;

    /* Free all listeners */
    AukListener* listener = obj->listeners;
    while (listener) {
        AukShared_Release(listener->listenerObject);
        /* ... free node ... */
    }

    FreeVec(obj);
}
```

## Update Notification Rules

### When Updates Are Sent

Updates are sent in these scenarios:

1. **Value Changed**: Set function modifies a value
   ```c
   project->SetName(project, "Different Name");  /* Sends update */
   ```

2. **Collection Modified**: Item added or removed
   ```c
   track->AddSound(track, sound);      /* Sends update */
   track->RemoveSound(track, sound);   /* Sends update */
   ```

3. **Reference Changed**: Pointer to another object changes
   ```c
   sound->SetSoundFile(sound, newFile); /* Sends update */
   ```

### When Updates Are NOT Sent

1. **Same Value**: Setting to current value
   ```c
   project->SetName(project, "Same");
   project->SetName(project, "Same");  /* No update - same value */
   ```

2. **Get Operations**: Reading data doesn't notify
   ```c
   const char* name = project->GetName(project);  /* No update */
   ```

3. **Failed Operations**: If Set returns failure
   ```c
   if (!project->SetName(project, NULL)) {
       /* Failed - no update sent */
   }
   ```

## Implementation Details

### AddListener

```c
int AukObject_AddListener(void* This, AukShared* listenerObject, AukUpdateCallback callback) {
    /* 1. Validate parameters */
    /* 2. Check if already registered (prevent duplicates) */
    /* 3. Allocate new listener node */
    /* 4. Retain shared reference */
    /* 5. Add to front of listener list */
}
```

### RemoveListener

```c
int AukObject_RemoveListener(void* This, void* listenerObject) {
    /* 1. Find listener in list by comparing pointers */
    /* 2. Remove from list */
    /* 3. Release shared reference */
    /* 4. Free listener node */
}
```

### SendUpdate

```c
void AukObject_SendUpdate(void* This) {
    /* 1. Iterate through all listeners */
    /* 2. Get listener object from shared pointer */
    /* 3. Call callback function with listener and this */
}
```

## Example: Complete GUI Integration

```c
/* GUI View Component */
typedef struct ProjectView {
    AukObject base;
    AukProject* project;
    /* GUI-specific members (window, labels, etc.) */
} ProjectView;

/* Update callback */
void ProjectView_OnProjectUpdate(void* listenerObject, void* modifiedObject) {
    ProjectView* view = (ProjectView*)listenerObject;
    AukProject* project = (AukProject*)modifiedObject;

    /* Update GUI labels */
    UpdateLabel(view->nameLabel, project->GetName(project));
    UpdateLabel(view->pathLabel, project->GetPath(project));
    UpdateLabel(view->trackCountLabel, project->GetTrackCount(project));
}

/* Initialize view */
void ProjectView_Init(ProjectView* view, AukProject* project) {
    AukShared* viewShared;

    view->project = project;

    /* Register as listener */
    viewShared = AukShared_Create(view, &view->base);
    project->base.AddListener(project, viewShared, ProjectView_OnProjectUpdate);
    AukShared_Release(viewShared);  /* Project retains reference */

    /* Initial UI update */
    ProjectView_OnProjectUpdate(view, project);
}

/* Cleanup view */
void ProjectView_Cleanup(ProjectView* view) {
    /* Unregister listener */
    if (view->project) {
        view->project->base.RemoveListener(view->project, view);
    }
}
```

## Benefits

1. **Clean Separation**: Document layer has zero GUI dependencies
2. **Automatic Propagation**: Changes automatically notify all interested parties
3. **Memory Safety**: Reference counting prevents dangling pointers
4. **Efficient**: Updates only sent when values actually change
5. **Flexible**: Any object can listen to any other object
6. **Multiple Listeners**: Many GUI components can watch same data object

## Modified Objects

All these objects send update notifications on modification:

- `AukProject` - name, path, preferences, add/remove tracks
- `AukTrack` - name, add/remove sounds, add envelope points
- `AukSound` - time range, file range, loop count, sound file reference
- `AukSoundFile` - filename, sample rate, channels, frame count

## See Also

- [examples/example_listener.c](examples/example_listener.c) - Complete working example
- [include/aukobject.h](include/aukobject.h) - Base object interface
- [src/aukobject.c](src/aukobject.c) - Listener implementation

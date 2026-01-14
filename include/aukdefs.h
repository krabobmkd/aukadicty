#ifndef AUKDEFS_H
#define AUKDEFS_H

/*
 * To be compatible with gcc2.95, struct typedef have to be set once. here.
 * no include should be made and it should be included first.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */

struct AukObject;
typedef struct AukObject AukObject;
typedef AukObject* AukObjectPtr;

typedef void (*AukObjectNewFunc)(AukObjectPtr* firstPtr);

struct ISerializer;
typedef struct ISerializer ISerializer;

struct AukMutex;
typedef struct AukMutex AukMutex;

struct AukListener;
typedef struct AukListener AukListener;

struct AukProject;
typedef struct AukProject AukProject;
typedef AukProject* AukProjectPtr;

struct AukArray;
typedef struct AukArray AukArray;
typedef AukArray* AukArrayPtr;

struct AukScalarArray;
typedef struct AukScalarArray AukScalarArray;

// - - - - - audio

struct AukTrack;
typedef struct AukTrack AukTrack;
typedef AukTrack* AukTrackPtr;

struct AukProjectPrefs;
typedef struct AukProjectPrefs AukProjectPrefs;
typedef AukProjectPrefs* AukProjectPrefsPtr;

struct AukAProject;
typedef struct AukAProject AukAProject;
typedef AukAProject* AukAProjectPtr;

/* Forward declarations */
typedef struct AukSound AukSound;
typedef AukSound* AukSoundPtr;

typedef struct AukSoundFile AukSoundFile;
typedef AukSoundFile* AukSoundFilePtr;


#ifdef __cplusplus
}
#endif

#endif /* SERIALIZER_H */

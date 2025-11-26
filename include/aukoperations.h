#ifndef AUKOPERATIONS_H
#define AUKOPERATIONS_H

/*
 * High-level data management operations for Aukadicty
 * Convenience functions for common operations on the project graph
 */

#include "aukproject.h"
#include "auktrack.h"
#include "auksound.h"
#include "auksoundfile.h"
#include "aukfixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sound management operations */

/* Create and add a sound to a track */
AukSound* AukOp_AddSoundToTrack(AukTrack* track, AukShared* soundFile,
                                 AukFixed startTime, AukFixed endTime);

/* Remove sound from its track and delete it */
int AukOp_RemoveSound(AukSound* sound);

/* Move sound from one track to another */
int AukOp_MoveSoundToTrack(AukSound* sound, AukTrack* newTrack);

/* Modify sound time range */
int AukOp_SetSoundTimeRange(AukSound* sound, AukFixed startTime, AukFixed endTime);

/* Modify sound file range */
int AukOp_SetSoundFileRange(AukSound* sound, unsigned long startFrame, unsigned long endFrame);

/* Set sound loop count */
int AukOp_SetSoundLoopCount(AukSound* sound, unsigned long loopCount);

/* Track management operations */

/* Create and add a track to project */
AukTrack* AukOp_AddTrackToProject(AukProject* project, const char* name);

/* Remove track from project and delete it */
int AukOp_RemoveTrack(AukTrack* track);

/* Find sound at specific time on track */
AukSound* AukOp_FindSoundAtTime(AukTrack* track, AukFixed time);

/* Get all sounds overlapping a time range on track */
unsigned long AukOp_GetSoundsInRange(AukTrack* track, AukFixed startTime, AukFixed endTime,
                                      AukSound** outSounds, unsigned long maxSounds);

/* Sound file management */

/* Create a shared sound file reference */
AukShared* AukOp_CreateSoundFile(const char* filename, unsigned long sampleRate,
                                 unsigned long channels, unsigned long frameCount);

/* Project operations */

/* Create new project with defaults */
AukProject* AukOp_CreateProject(const char* name, const char* path);

/* Get total project duration (longest track end time) */
AukFixed AukOp_GetProjectDuration(AukProject* project);

#ifdef __cplusplus
}
#endif

#endif /* AUKOPERATIONS_H */

/*
 * Basic example of using Aukadicty library
 * Demonstrates creating a project, adding tracks and sounds
 */

#include <aukadicty.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

int main(void) {
    AukProjectPtr projectPtr = NULL;
    AukProject* project;
    AukTrack* track1;
    AukTrack* track2;
    AukSoundFilePtr soundFile1 = NULL;
    AukSound* sound1;
    AukSound* sound2;
    AukFixed duration;

    /* Create a new project */
    AukProject_New(&projectPtr);
    project = projectPtr;
    if (!project) {
        printf("Failed to create project\n");
        return 1;
    }

    /* Set project properties */
    project->SetName(project, "My First Project");
    project->SetPath(project, "Work:");
    AukProject_SetPreferences(project, 44100, 16);

    /* Create tracks in the project */
    track1 = project->CreateTrack(project);
    track2 = project->CreateTrack(project);

    if (!track1 || !track2) {
        printf("Failed to create tracks\n");
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }
    AukTrack_SetName(track1, "Vocals");
    AukTrack_SetName(track2, "Music");


    /* Create a sound file reference */
    AukSoundFile_New(&soundFile1);
    if (!soundFile1) {
        printf("Failed to create sound file\n");
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }
    AukSoundFile_SetFilename(soundFile1, "sounds/sample1.wav");
    AukSoundFile_SetProperties(soundFile1, 44100, 2, 88200);

    /* Create sounds on tracks */
    sound1 = track1->CreateSound(track1, soundFile1,
                                 AukFixed_FromInt(0),    /* Start at 0 seconds */
                                 AukFixed_FromInt(5));   /* End at 5 seconds */

    sound2 = track2->CreateSound(track2, soundFile1,
                                 AukFixed_FromInt(2),    /* Start at 2 seconds */
                                 AukFixed_FromInt(8));   /* End at 8 seconds */

    if (!sound1 || !sound2) {
        printf("Failed to add sounds\n");
        AukObjectPtr_Release((AukObjectPtr*)&soundFile1);
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }

    /* Release our reference to sound file (sounds now own it) */
    AukObjectPtr_Release((AukObjectPtr*)&soundFile1);

    /* Set sound properties */
    sound1->SetLoopCount(sound1, 2);  /* Loop twice */

    /* Add envelope points to track1 */
    track1->CreateEnvelopePoint(track1,
                                AukFixed_FromDouble(-0.25),
                                AukFixed_FromInt(1));  /* Full volume at start */
    track1->CreateEnvelopePoint(track1,
                                AukFixed_FromInt(5),
                                AukFixed_FromFraction(1, 2));  /* Half volume at 5 seconds */

    /* Get project duration */
    duration = project->GetDuration(project);
    printf("Project duration: %ld seconds\n", AukFixed_ToInt(duration));

    /* Save project to JSON file */
    if (project->Save(project, "my_project.auk")) {
        printf("Project saved successfully\n");
    } else {
        printf("Failed to save project\n");
    }

    /* Display project info */
    printf("Project: %s\n", project->GetName(project));
    printf("Tracks: %lu\n", project->GetTrackCount(project));
    printf("Track 1: %s, Sounds: %lu\n",
           AukTrack_GetName(track1),
           track1->GetSoundCount(track1));
    printf("Track 2: %s, Sounds: %lu\n",
           AukTrack_GetName(track2),
           track2->GetSoundCount(track2));

    /* Clean up - this will delete all tracks, sounds, and sound files */
    AukObjectPtr_Release((AukObjectPtr*)&projectPtr);

    printf("Example completed successfully\n");
    return 0;
}

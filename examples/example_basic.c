/*
 * Basic example of using Aukadicty library
 * Demonstrates creating a project, adding tracks and sounds
 */

#include <aukadicty.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

int main(void) {
    AukProject* project;
    AukTrack* track1;
    AukTrack* track2;
    AukShared* soundFile1;
    AukSound* sound1;
    AukSound* sound2;
    AukFixed duration;

    /* Create a new project */
    project = AukOp_CreateProject("My First Project", "Work:");
    if (!project) {
        printf("Failed to create project\n");
        return 1;
    }

    /* Set project preferences */
    AukProject_SetPreferences(project, 44100, 16);

    /* Add tracks to the project */
    track1 = AukOp_AddTrackToProject(project, "Vocals");
    track2 = AukOp_AddTrackToProject(project, "Music");

    if (!track1 || !track2) {
        printf("Failed to create tracks\n");
        AukProject_Delete(project);
        return 1;
    }

    /* Create a sound file reference */
    soundFile1 = AukOp_CreateSoundFile("sounds/sample1.wav", 44100, 2, 88200);
    if (!soundFile1) {
        printf("Failed to create sound file\n");
        AukProject_Delete(project);
        return 1;
    }

    /* Add sounds to tracks */
    sound1 = AukOp_AddSoundToTrack(track1, soundFile1,
                                    AukFixed_FromInt(0),    /* Start at 0 seconds */
                                    AukFixed_FromInt(5));   /* End at 5 seconds */

    sound2 = AukOp_AddSoundToTrack(track2, soundFile1,
                                    AukFixed_FromInt(2),    /* Start at 2 seconds */
                                    AukFixed_FromInt(8));   /* End at 8 seconds */

    if (!sound1 || !sound2) {
        printf("Failed to add sounds\n");
        AukShared_Release(soundFile1);
        AukProject_Delete(project);
        return 1;
    }

    /* Release our reference to sound file (sounds now own it) */
    AukShared_Release(soundFile1);

    /* Set sound properties */
    AukOp_SetSoundLoopCount(sound1, 2);  /* Loop twice */

    /* Add envelope points to track1 */
    track1->AddEnvelopePoint(track1,
                             AukFixed_FromInt(0),
                             AukFixed_FromInt(1));  /* Full volume at start */
    track1->AddEnvelopePoint(track1,
                             AukFixed_FromInt(5),
                             AukFixed_FromFraction(1, 2));  /* Half volume at 5 seconds */

    /* Get project duration */
    duration = AukOp_GetProjectDuration(project);
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
    AukProject_Delete(project);

    printf("Example completed successfully\n");
    return 0;
}

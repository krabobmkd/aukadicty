/*
 * Example of loading an existing Aukadicty project
 */

#include <aukadicty.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

int main(int argc, char** argv) {
    AukProject* project;
    AukTrack* track;
    AukSound* sound;
    unsigned long i, j;
    unsigned long trackCount, soundCount;
    const char* filename;

    if (argc < 2) {
        printf("Usage: %s <project_file.auk>\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    /* Load project from JSON file */
    project = AukJson_LoadProject(filename);
    if (!project) {
        printf("Failed to load project from %s\n", filename);
        return 1;
    }

    printf("Loaded project: %s\n", project->GetName(project));
    printf("Sample rate: %lu Hz\n", project->prefs.sampleRate);
    printf("Max tracks: %lu\n", project->prefs.maxTracks);

    /* Display all tracks and their sounds */
    trackCount = project->GetTrackCount(project);
    printf("\nTracks: %lu\n", trackCount);

    for (i = 0; i < trackCount; i++) {
        track = project->GetTrack(project, i);
        if (track) {
            printf("\nTrack %lu: %s\n", i + 1, AukTrack_GetName(track));

            soundCount = track->GetSoundCount(track);
            printf("  Sounds: %lu\n", soundCount);

            for (j = 0; j < soundCount; j++) {
                sound = track->GetSound(track, j);
                if (sound) {
                    AukSoundFile* soundFile = (AukSoundFile*)AukShared_GetObject(sound->soundFile);

                    printf("    Sound %lu:\n", j + 1);
                    if (soundFile) {
                        printf("      File: %s\n", soundFile->GetFilename(soundFile));
                        printf("      Sample rate: %lu Hz\n", soundFile->sampleRate);
                        printf("      Channels: %lu\n", soundFile->channels);
                    }
                    printf("      Start: %ld ms\n",
                           AukFixed_ToInt(AukFixed_Mul(sound->startTime, AukFixed_FromInt(1000))));
                    printf("      End: %ld ms\n",
                           AukFixed_ToInt(AukFixed_Mul(sound->endTime, AukFixed_FromInt(1000))));
                    printf("      Loop count: %lu\n", sound->loopCount);
                }
            }
        }
    }

    /* Clean up */
    AukProject_Delete(project);

    return 0;
}

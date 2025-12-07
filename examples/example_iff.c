/*
 * Example: IFF Binary Serialization
 * Demonstrates saving and loading projects using Amiga IFF format
 */

#include <stdio.h>
#include "aukadicty.h"
#include "aukiffserializer.h"
#include "auktyperegistry.h"
#include "aukfixed.h"
#include <proto/dos.h>

void PrintProjectInfo(AukProject* project) {
    unsigned int i, j;
    AukTrack* track = NULL;

    const char* projectName;
    unsigned int trackCount, soundCount;
    AukFixed duration;

    projectName = project->GetName(project);
    trackCount = project->GetTrackCount(project);
    duration = project->GetDuration(project);

    printf("\n=== Project Information ===\n");
    printf("Name: %s\n", projectName ? projectName : "(unnamed)");
    printf("Track count: %u\n", trackCount);
    printf("Duration: %ld seconds\n", AukFixed_ToInt(duration));

    /* Print track information */
    for (i = 0; i < trackCount; i++) {
        project->GetTrack(project, &track, i);
        if (track) {
            soundCount = track->GetSoundCount(track);
            printf("\nTrack %u: %s\n", i + 1, AukTrack_GetName(track));
            printf("  Sounds: %u\n", soundCount);

            /* Print sound information */
            for (j = 0; j < soundCount; j++) {
                AukSound* sound=NULL;
                track->GetSound(track,&sound, j);
                if (sound) {
                    AukSoundFile* soundFile = sound->soundFile;
                    printf("    Sound %u:\n", j + 1);
                    if (soundFile) {
                        printf("      File: %s\n", soundFile->GetFilename(soundFile));
                        printf("      Sample rate: %lu Hz\n", soundFile->sampleRate);
                        printf("      Channels: %lu\n", soundFile->channels);
                    }
                    printf("      Start time: %ld.%03ld seconds\n",
                           AukFixed_ToInt(sound->startTime),
                          /* AukFixed_ToMilliseconds(sound->startTime) % 1000*/0);
                    printf("      End time: %ld.%03ld seconds\n",
                           AukFixed_ToInt(sound->endTime),
                           /*AukFixed_ToMilliseconds(sound->endTime) % 1000*/0);
                    if (sound->loopCount > 0) {
                        printf("      Loop count: %lu\n", sound->loopCount);
                    }
                    AukObjectPtr_Release(&sound);
                }
            }
        }
    }
    AukObjectPtr_Release((AukObjectPtr*)&track);

    printf("\n");
}

int SaveProjectIFF(AukProject** project, const char* filename) {
    BPTR file;
    ISerializer* ser;

    printf("Saving project to IFF file: %s\n", filename);

    /* Open file for writing */
    file = Open((STRPTR)filename, MODE_NEWFILE);
    if (!file) {
        printf("ERROR: Failed to open file for writing\n");
        return 0;
    }

    /* Create IFF writer serializer */
    ser = AukIFFSerializer_CreateWriter(file);
    if (!ser) {
        printf("ERROR: Failed to create IFF serializer\n");
        Close(file);
        return 0;
    }

    /* Serialize the project */
    ser->t_object(ser,"project",(AukObject**)project);

    /* Finalize IFF (writes correct FORM size) */
    if (!AukIFFSerializer_Finalize(ser)) {
        printf("ERROR: Failed to finalize IFF file\n");
        ser->Destroy(ser);
        Close(file);
        return 0;
    }

    /* Clean up */
    ser->Destroy(ser);
    Close(file);

    printf("Project saved successfully!\n");
    return 1;
}

int LoadProjectIFF(AukProjectPtr* projectPtr, const char* filename) {
    BPTR file;
    ISerializer* ser;
    const TypeNameToContructor* typeRegistry;

    printf("Loading project from IFF file: %s\n", filename);

    /* Open file for reading */
    file = Open((STRPTR)filename, MODE_OLDFILE);
    if (!file) {
        printf("ERROR: Failed to open file for reading\n");
        return 0;
    }

    /* Get type registry */
    typeRegistry = AukProject_GetTypeRegistry();

    /* Create IFF reader serializer */
    ser = AukIFFSerializer_CreateReader(file, typeRegistry);
    if (!ser) {
        printf("ERROR: Failed to create IFF reader (invalid file format?)\n");
        Close(file);
        return 0;
    }

    /* Deserialize the project */
    ser->t_object(ser, "project", (AukObjectPtr*)projectPtr);

    /* Clean up */
    ser->Destroy(ser);
    Close(file);

    if (!*projectPtr) {
        printf("ERROR: Failed to deserialize project\n");
        return 0;
    }

    printf("Project loaded successfully!\n");
    return 1;
}

int main(void) {
    AukProjectPtr projectPtr = NULL;
    AukProject* project;
    AukTrack* track1;
    AukTrack* track2;
    AukSoundFilePtr soundFile1 = NULL;
    AukSoundFilePtr soundFile2 = NULL;
    AukSound* sound1;
    AukSound* sound2;
    AukSound* sound3;
    AukProjectPtr loadedProjectPtr = NULL;
    AukProject* loadedProject;

    printf("=== IFF Binary Serialization Example ===\n\n");

    /* ========== Create Project ========== */
    printf("Creating project...\n");

    AukProject_New(&projectPtr);
    project = projectPtr;
    if (!project) {
        printf("Failed to create project\n");
        return 1;
    }

    /* Set project properties */
    project->SetName(project, "IFF Demo Project");
    project->SetPath(project, "Work:");
    AukProject_SetPreferences(project, 48000, 32);

    /* Create tracks */
    track1 = project->CreateTrack(project);
    track2 = project->CreateTrack(project);

    if (!track1 || !track2) {
        printf("Failed to create tracks\n");
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }

    AukTrack_SetName(track1, "Vocals");
    AukTrack_SetName(track2, "Instruments");

    /* Create sound files */
    AukSoundFile_New(&soundFile1);
    if (!soundFile1) {
        printf("Failed to create sound file 1\n");
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }
    AukSoundFile_SetFilename(soundFile1, "sounds/vocals.wav");
    AukSoundFile_SetProperties(soundFile1, 48000, 2, 144000);

    AukSoundFile_New(&soundFile2);
    if (!soundFile2) {
        printf("Failed to create sound file 2\n");
        AukObjectPtr_Release((AukObjectPtr*)&soundFile1);
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }
    AukSoundFile_SetFilename(soundFile2, "sounds/guitar.8svx");
    AukSoundFile_SetProperties(soundFile2, 48000, 1, 96000);

    /* Create sounds on tracks */
    sound1 = track1->CreateSound(track1, soundFile1,
                                 AukFixed_FromInt(0),
                                 AukFixed_FromInt(3));

    sound2 = track2->CreateSound(track2, soundFile2,
                                 AukFixed_FromDouble(0.5),
                                 AukFixed_FromDouble(4.5));

    sound3 = track2->CreateSound(track2, soundFile2,
                                 AukFixed_FromInt(5),
                                 AukFixed_FromInt(8));

    if (!sound1 || !sound2 || !sound3) {
        printf("Failed to create sounds\n");
        AukObjectPtr_Release((AukObjectPtr*)&soundFile1);
        AukObjectPtr_Release((AukObjectPtr*)&soundFile2);
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }

    /* Set sound properties */
    sound1->SetLoopCount(sound1, 1);
    sound2->SetFileRange(sound2, 0, 48000);  /* First second of file */

    // /* Add envelope points */
    track1->AddEnvelopePoint(track1, AukFixed_FromInt(0), AukFixed_FromInt(1));
    track1->AddEnvelopePoint(track1, AukFixed_FromInt(3), AukFixed_FromFraction(1, 2));

    track2->AddEnvelopePoint(track2, AukFixed_FromInt(0), AukFixed_FromFraction(3, 4));
    track2->AddEnvelopePoint(track2, AukFixed_FromInt(4), AukFixed_FromInt(1));
    track2->AddEnvelopePoint(track2, AukFixed_FromInt(8), AukFixed_FromFraction(1, 4));

    // /* Release our references to sound files */
    AukObjectPtr_Release((AukObjectPtr*)&soundFile1);
    AukObjectPtr_Release((AukObjectPtr*)&soundFile2);

    // /* Print original project info */
    PrintProjectInfo(project);

    /* ========== Save to IFF ========== */
    if (!SaveProjectIFF(&projectPtr, "demo_project.aup")) {
        printf("Failed to save project\n");
        AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
        return 1;
    }

    /* ========== Release original project ========== */
    printf("\nReleasing original project from memory...\n");
    AukObjectPtr_Release((AukObjectPtr*)&projectPtr);
    printf("Original project released.\n");

    /* ========== Load from IFF ========== */
    printf("\n");
    if (!LoadProjectIFF(&loadedProjectPtr, "demo_project.aup")) {
        printf("Failed to load project\n");
        return 1;
    }

    loadedProject = loadedProjectPtr;

    /* Print loaded project info */
    printf("\nLoaded project information:\n");
    PrintProjectInfo(loadedProject);

    /* ========== Verify Data Integrity ========== */
    printf("=== Verifying Data Integrity ===\n");

    if (AukString_Compare(loadedProject->GetName(loadedProject), "IFF Demo Project") == 0) {
        printf("✓ Project name matches\n");
    } else {
        printf("✗ Project name mismatch\n");
    }

    if (loadedProject->GetTrackCount(loadedProject) == 2) {
        printf("✓ Track count matches\n");
    } else {
        printf("✗ Track count mismatch\n");
    }

    AukTrack* loadedTrack = NULL;
    loadedProject->GetTrack(loadedProject, &loadedTrack, 0);
    if (loadedTrack && AukString_Compare(AukTrack_GetName(loadedTrack), "Vocals") == 0) {
        printf("✓ First track name matches\n");
    } else {
        printf("✗ First track name mismatch\n");
    }

    if (loadedTrack && loadedTrack->GetSoundCount(loadedTrack) == 1) {
        printf("✓ First track sound count matches\n");
    } else {
        printf("✗ First track sound count mismatch\n");
    }
    AukObjectPtr_Release((AukObjectPtr*)&loadedTrack);

    loadedProject->GetTrack(loadedProject, &loadedTrack, 1);
    if (loadedTrack && loadedTrack->GetSoundCount(loadedTrack) == 2) {
        printf("✓ Second track sound count matches\n");
    } else {
        printf("✗ Second track sound count mismatch\n");
    }
    AukObjectPtr_Release((AukObjectPtr*)&loadedTrack);

    printf("\n=== IFF Serialization Test Complete ===\n");

    /* Clean up */
    AukObjectPtr_Release((AukObjectPtr*)&loadedProjectPtr);

    printf("\nIFF example completed successfully!\n");
    return 0;
}

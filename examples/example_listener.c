/*
 * Example demonstrating the listener/observer pattern
 * Shows how GUI components can subscribe to data model changes
 */

#include <aukadicty.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

/* Simulated GUI object that listens to project changes */
typedef struct sMockGUIView {
    AukObject base;  /* Must be first for inheritance */
    char* viewName;
    int updateCount;
} MockGUIView;

/* Update callback for GUI view */
void MockGUIView_OnUpdate(void* listenerObject, void* modifiedObject) {
    MockGUIView* view = (MockGUIView*)listenerObject;
    AukObject* obj = (AukObject*)modifiedObject;

    if (view && obj) {
        view->updateCount++;
        printf("[%s] Received update from %s (update #%d)\n",
               view->viewName,
               obj->GetTypeName(obj),
               view->updateCount);
    }
}
void MockGUIView_Delete(MockGUIView* view) ;

/* Create mock GUI view */
void MockGUIView_New(MockGUIView**ptr,const char* name) {
    MockGUIView* view = (MockGUIView*)AllocVec(sizeof(MockGUIView), MEMF_CLEAR);
    if (view) {
        AukObject_Init(&view->base);
        view->base.Delete = MockGUIView_Delete;
        view->viewName = AukString_Duplicate(name);
        view->updateCount = 0;
        AukObjectPtr_Set(ptr,&view->base);
    }
}

/* Delete mock GUI view */
void MockGUIView_Delete(MockGUIView* view) {
    if (view) {
        if (view->viewName) {
            AukString_Free(view->viewName);
        }
        AukObject_Delete(view);
    }
}

int main(void) {
    AukProjectPtr projectPtr = NULL;
    AukProject* project= NULL;
    AukTrack* track= NULL;
    AukSound* sound= NULL;
    AukSoundFilePtr soundFile = NULL;
//    MockGUIView* projectView;
//    MockGUIView* trackView;
//    MockGUIView* soundView;
    MockGUIView *projectView = NULL;
    MockGUIView *trackView = NULL;
    MockGUIView *soundView = NULL;

    printf("=== Listener Pattern Example ===\n\n");

    /* Create project and GUI views */
    AukProject_New(&projectPtr);
    project = projectPtr;
    project->SetName(project, "Test Project");
    project->SetPath(project, "Work:");
    track = project->CreateTrack(project);
    AukTrack_SetName(track, "Track 1");
    /* Create GUI view objects (would be actual GUI widgets in real app) */
    MockGUIView_New(&projectView,"ProjectView");
    MockGUIView_New(&trackView,"TrackView");
    MockGUIView_New(&soundView,"SoundView");

    /* Register listeners */
    printf("Registering listeners...\n\n");
    project->base.AddListener(project, &projectView->base, MockGUIView_OnUpdate);
    track->base.AddListener(track, &trackView->base, MockGUIView_OnUpdate);

    /* Modify project - should trigger projectView update */
    printf("Setting project name...\n");
    project->SetName(project, "Updated Project");
    printf("\n");

    /* Modify track - should trigger trackView update */
    printf("Setting track name...\n");
    AukTrack_SetName(track, "Vocals");
    printf("\n");

    /* Add sound to track - should trigger trackView update */
    printf("Adding sound to track...\n");
    AukSoundFile_New(&soundFile);
    AukSoundFile_SetFilename(soundFile, "test.wav");
    AukSoundFile_SetProperties(soundFile, 44100, 2, 88200);
    sound = track->CreateSound(track, soundFile,
                               AukFixed_FromInt(0),
                               AukFixed_FromInt(5));

    /* Register listener on sound */
    sound->base.AddListener(sound, &soundView->base, MockGUIView_OnUpdate);
    printf("\n");

    /* Modify sound - should trigger soundView update */
    printf("Changing sound time range...\n");
    sound->SetTimeRange(sound, AukFixed_FromInt(1), AukFixed_FromInt(6));
    printf("\n");

    /* Modify sound loop count - should trigger soundView update */
    printf("Setting sound loop count...\n");
    sound->SetLoopCount(sound, 3);
    printf("\n");

    /* Set same value again - should NOT trigger update */
    printf("Setting same loop count again (should not trigger update)...\n");
    sound->SetLoopCount(sound, 3);
    printf("\n");

    /* Add envelope point - should trigger trackView update */
    printf("Adding envelope point to track...\n");
    track->CreateEnvelopePoint(track, AukFixed_FromInt(0), AukFixed_FromInt(1));
    printf("\n");

    /* Summary */
    printf("=== Update Summary ===\n");
    printf("Project updates received by ProjectView: %d\n", projectView->updateCount);
    printf("Track updates received by TrackView: %d\n", trackView->updateCount);
    printf("Sound updates received by SoundView: %d\n", soundView->updateCount);
    printf("\n");

    /* Unregister listeners */
    printf("Unregistering listeners...\n");
    project->base.RemoveListener(&project->base, &projectView->base);
    track->base.RemoveListener(&track->base, &trackView->base);
    sound->base.RemoveListener(&sound->base, &soundView->base);

    /* Modify after unregistering - should sNOT trigger updates */
    printf("Modifying after unregister (should not trigger updates)...\n");
    project->SetName(project, "Final Name");
    AukTrack_SetName(track, "Final Track Name");
    printf("\n");

    printf("=== Final Summary ===\n");
    printf("Project updates: %d (expected: 1)\n", projectView->updateCount);
    printf("Track updates: %d (expected: 3)\n", trackView->updateCount);
    printf("Sound updates: %d (expected: 2)\n", soundView->updateCount);

    /* Cleanup */
    AukObjectPtr_Release(&projectView);
    AukObjectPtr_Release(&trackView);
    AukObjectPtr_Release(&soundView);
    AukObjectPtr_Release((AukObjectPtr*)&soundFile);
    AukObjectPtr_Release((AukObjectPtr*)&projectPtr);

    printf("\nExample completed successfully\n");
    return 0;
}

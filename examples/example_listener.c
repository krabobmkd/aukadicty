/*
 * Example demonstrating the listener/observer pattern
 * Shows how GUI components can subscribe to data model changes
 */

#include <aukadicty.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

/* Simulated GUI object that listens to project changes */
typedef struct MockGUIView {
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

/* Create mock GUI view */
MockGUIView* MockGUIView_New(const char* name) {
    MockGUIView* view = (MockGUIView*)AllocVec(sizeof(MockGUIView), MEMF_CLEAR);
    if (view) {
        AukObject_Init(&view->base);
        view->viewName = AukString_Duplicate(name);
        view->updateCount = 0;
    }
    return view;
}

/* Delete mock GUI view */
void MockGUIView_Delete(MockGUIView* view) {
    if (view) {
        if (view->viewName) {
            AukString_Free(view->viewName);
        }
        FreeVec(view);
    }
}

int main(void) {
    AukProject* project;
    AukTrack* track;
    AukSound* sound;
    AukShared* soundFile;
    MockGUIView* projectView;
    MockGUIView* trackView;
    MockGUIView* soundView;
    AukShared* projectViewShared;
    AukShared* trackViewShared;
    AukShared* soundViewShared;

    printf("=== Listener Pattern Example ===\n\n");

    /* Create project and GUI views */
    project = AukOp_CreateProject("Test Project", "Work:");
    track = AukOp_AddTrackToProject(project, "Track 1");

    /* Create GUI view objects (would be actual GUI widgets in real app) */
    projectView = MockGUIView_New("ProjectView");
    trackView = MockGUIView_New("TrackView");
    soundView = MockGUIView_New("SoundView");

    /* Wrap views in shared pointers for listener registration */
    projectViewShared = AukShared_Create(projectView, &projectView->base);
    trackViewShared = AukShared_Create(trackView, &trackView->base);
    soundViewShared = AukShared_Create(soundView, &soundView->base);

    /* Register listeners */
    printf("Registering listeners...\n\n");
    project->base.AddListener(project, projectViewShared, MockGUIView_OnUpdate);
    track->base.AddListener(track, trackViewShared, MockGUIView_OnUpdate);

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
    soundFile = AukOp_CreateSoundFile("test.wav", 44100, 2, 88200);
    sound = AukOp_AddSoundToTrack(track, soundFile,
                                   AukFixed_FromInt(0),
                                   AukFixed_FromInt(5));

    /* Register listener on sound */
    sound->base.AddListener(sound, soundViewShared, MockGUIView_OnUpdate);
    printf("\n");

    /* Modify sound - should trigger soundView update */
    printf("Changing sound time range...\n");
    AukOp_SetSoundTimeRange(sound, AukFixed_FromInt(1), AukFixed_FromInt(6));
    printf("\n");

    /* Modify sound loop count - should trigger soundView update */
    printf("Setting sound loop count...\n");
    AukOp_SetSoundLoopCount(sound, 3);
    printf("\n");

    /* Set same value again - should NOT trigger update */
    printf("Setting same loop count again (should not trigger update)...\n");
    AukOp_SetSoundLoopCount(sound, 3);
    printf("\n");

    /* Add envelope point - should trigger trackView update */
    printf("Adding envelope point to track...\n");
    track->AddEnvelopePoint(track, AukFixed_FromInt(0), AukFixed_FromInt(1));
    printf("\n");

    /* Summary */
    printf("=== Update Summary ===\n");
    printf("Project updates received by ProjectView: %d\n", projectView->updateCount);
    printf("Track updates received by TrackView: %d\n", trackView->updateCount);
    printf("Sound updates received by SoundView: %d\n", soundView->updateCount);
    printf("\n");

    /* Unregister listeners */
    printf("Unregistering listeners...\n");
    project->base.RemoveListener(project, projectView);
    track->base.RemoveListener(track, trackView);
    sound->base.RemoveListener(sound, soundView);

    /* Modify after unregistering - should NOT trigger updates */
    printf("Modifying after unregister (should not trigger updates)...\n");
    project->SetName(project, "Final Name");
    AukTrack_SetName(track, "Final Track Name");
    printf("\n");

    printf("=== Final Summary ===\n");
    printf("Project updates: %d (expected: 1)\n", projectView->updateCount);
    printf("Track updates: %d (expected: 3)\n", trackView->updateCount);
    printf("Sound updates: %d (expected: 2)\n", soundView->updateCount);

    /* Cleanup */
    AukShared_Release(projectViewShared);
    AukShared_Release(trackViewShared);
    AukShared_Release(soundViewShared);
    AukShared_Release(soundFile);

    MockGUIView_Delete(projectView);
    MockGUIView_Delete(trackView);
    MockGUIView_Delete(soundView);

    AukProject_Delete(project);

    printf("\nExample completed successfully\n");
    return 0;
}

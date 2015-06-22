#pragma once

#include "Shadertoy.h"
#include "ImguiHelper.h"

// Platform Functions
typedef double(*TimeFunc)(void);
typedef void(*StopAudioFunc)(void*);
typedef void*(*InitAudioOutputFunc)(int, int, int, int);
typedef void*(*InitAudioInputFunc)(int, int, int, int);

// Types
struct ShadertoyTestResource {
    ResourceType type;
    const char *filename[6];
};

struct ShadertoyTestPass {
    const char *shader;
    ShadertoyTestResource channels[SHADERTOY_MAX_CHANNELS];
};

// TODO(jose): add link to shadertoy website + comment or what to look for on the test
struct ShadertoyTest {
    const char *title;
    ShadertoyTestPass image;
    ShadertoyTestPass sound;
    double start_time;
};

struct ShadertoyTestInfo {
    ShadertoyTest *test;
    int count;
};

// Helper utils
void LoadTexture(ShadertoyState *state, char *filename[], ShadertoyPass pass, int channel_id);
void LoadTexture(ShadertoyState *state, const char *filename, ShadertoyPass pass, int channel_id);
void LoadTestChannel(ShadertoyTestResource *channel_data, ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs, ShadertoyPass pass, int channel_id);
void LoadTest(ShadertoyTest *test, ShadertoyConfig *config, ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs);

// TODO(jose): Nasty helper for audio data, to be refined on next port
void GetAudioInfo(int channel_id, int *size, void **data);

// Debug GUI
bool GUITestSelection(bool show_selection, ShadertoyTestInfo *info, ShadertoyTest **current_test);
void GUITestOverlay(bool show_overlay, ShadertoyTest *test, ShadertoyState *state, ShadertoyInputs *inputs);

// Test lifecycle
ShadertoyTestInfo* TestInit(TimeFunc time_func, StopAudioFunc stop_audio_func, InitAudioOutputFunc init_audio_output_func, InitAudioInputFunc init_audio_input_func, void *window);
void TestBegin();
void TestEnd();
void TestShutdown();


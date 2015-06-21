#include "ShadertoyTestHelper.h"

#include "ImguiHelper.h"
#include "DataPath.h"

#pragma warning (push)
#pragma warning( disable : 4456 )
#pragma warning( disable : 4457 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4459 )
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_vorbis.c"
#pragma warning (pop)

struct ImageFile {
    char filepath[2048];
};

struct AudioFile {
    char filepath[2048];
    float *samples;
    int channels;
    int samples_count;
};

// We should never use these functions
double DummyTime() {
    assert(false);
    return 0.0f;
}

void DummyStopAudio(void*) {
    assert(false);
}

void* DummyInitAudio(int, int, int, int) {
    assert(false);
    return NULL;
}

// Global data for test
static ImageFile image_files[SHADERTOY_MAX_CHANNELS][SHADERTOY_PASSES_COUNT];
static AudioFile audio_files[SHADERTOY_MAX_CHANNELS];
static TimeFunc time_now = DummyTime;
static StopAudioFunc stop_audio = DummyStopAudio;
static InitAudioOutputFunc init_audio_output = DummyInitAudio;
static InitAudioInputFunc init_audio_input = DummyInitAudio;

void GetAudioInfo(int channel_id, int *size, void **data) {
    AudioFile *target = &audio_files[channel_id];
    *size = target->samples_count * sizeof(float);
    *data = target->samples;
}

// TODO(jose): I don´t like this helper API
void LoadTexture(ShadertoyState *state, char *filename[], ShadertoyPass pass, int channel_id) {
    int comp = STBI_rgb;
    Texture tex[6];
    for (int i = 0; i < 6; ++i) {
        tex[i].data = stbi_load(filename[i], &tex[i].width, &tex[i].height, &comp, 0);
        tex[i].format = comp == STBI_rgb ? SHADERTOY_TEXTURE_FORMAT_RGB : SHADERTOY_TEXTURE_FORMAT_RGBA;
    }
    ShadertoyLoadCubemap(state, tex, pass, channel_id);
    for (int i = 0; i < 6; ++i) {
        stbi_image_free(tex[i].data);
    }
}

void LoadTexture(ShadertoyState *state, const char *filename, ShadertoyPass pass, int channel_id) {
    int comp = STBI_rgb;
    Texture tex;
    tex.data = stbi_load(filename, &tex.width, &tex.height, &comp, 0);
    tex.format = comp == STBI_rgb ? SHADERTOY_TEXTURE_FORMAT_RGB : SHADERTOY_TEXTURE_FORMAT_RGBA;
    ShadertoyLoadTexture(state, &tex, pass, channel_id);
    stbi_image_free(tex.data);
}

void GetFilePath(char *dest, const char *filename) {
    static const char *working_directory = GetDataPath();
    strcat(dest, working_directory);
    strcat(dest, GetPathSeparator());
    strcat(dest, filename);
}

void LoadTestChannel(ShadertoyTestResource *channel_data, ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs, ShadertoyPass pass, int channel_id) {

    ImageFile *image = &image_files[channel_id][pass];
    memset(image->filepath, 0, sizeof(image->filepath));

    switch (channel_data->type) {
    case SHADERTOY_RESOURCE_TEXTURE: {
        GetFilePath(image->filepath, channel_data->filename[0]);
        LoadTexture(state, image->filepath, pass, channel_id);
        break;
    }
    case SHADERTOY_RESOURCE_CUBE_MAP: {
        char *cubemap_paths[6];
        for (int j = 0; j < 6; ++j) {
            cubemap_paths[j] = new char[1024];
            GetFilePath(cubemap_paths[j], channel_data->filename[j]);
            if (j == 0) {
                strcpy(image->filepath, cubemap_paths[j]);
            }
        }
        LoadTexture(state, cubemap_paths, pass, channel_id);
        for (int j = 0; j < 6; ++j) {
            delete[] cubemap_paths[j];
        }
        break;
    }
    case SHADERTOY_RESOURCE_MUSIC: {
        AudioFile *file = &audio_files[channel_id];
        memset(file->filepath, 0, sizeof(file->filepath));
        GetFilePath(file->filepath, channel_data->filename[0]);

        int ogg_error;
        stb_vorbis *ogg_data = stb_vorbis_open_filename(file->filepath, &ogg_error, NULL);
        file->channels = ogg_data->channels;
        file->samples_count = stb_vorbis_stream_length_in_samples(ogg_data) * ogg_data->channels;
        file->samples = (float*)malloc(sizeof(float) * file->samples_count);
        stb_vorbis_get_samples_float_interleaved(ogg_data, ogg_data->channels, file->samples, file->samples_count);

        ShadertoyAudio music = { &inputs->audio_played_samples[channel_id], &file->samples, (int)ogg_data->sample_rate, ogg_data->channels, file->samples_count / ogg_data->channels };
        ShadertoyLoadAudio(state, &music, SHADERTOY_IMAGE_PASS, channel_id);

        outputs->music_data_param[channel_id] = init_audio_output(ogg_data->channels, ogg_data->sample_rate, 32, file->samples_count * sizeof(float));
        stb_vorbis_close(ogg_data);

        break;
    }
    case SHADERTOY_RESOURCE_KEYBOARD:
    case SHADERTOY_RESOURCE_NONE:
        break;
    }
}

void LoadTest(ShadertoyTest *test, ShadertoyConfig *config, ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs) {
    if (state->sound_enable) {
        stop_audio(outputs->sound_data_param);
    }

    for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
        if (state->music_enabled_channel[i]) {
            AudioFile *audio = &audio_files[i];
            stop_audio(outputs->music_data_param[i]);
            free(audio->samples);
        }
    }

    ShadertoyFree(state, inputs, outputs);
    ShadertoyInit(config, state, inputs, outputs);

    // Image pass
    ResourceType image_resources[SHADERTOY_MAX_CHANNELS] = {};
    for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
        ShadertoyTestResource *channel_data = &test->image.channels[i];
        LoadTestChannel(channel_data, state, inputs, outputs, SHADERTOY_IMAGE_PASS, i);
        image_resources[i] = channel_data->type;
    }
    ShadertoyLoadShader(state, test->image.shader, image_resources[0], image_resources[1], image_resources[2], image_resources[3], SHADERTOY_IMAGE_PASS);

    if (test->sound.shader != NULL) {
        ResourceType sound_resources[SHADERTOY_MAX_CHANNELS] = {};
        for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
            ShadertoyTestResource *channel_data = &test->sound.channels[i];
            LoadTestChannel(channel_data, state, inputs, outputs, SHADERTOY_SOUND_PASS, i);
            sound_resources[i] = channel_data->type;
        }
        ShadertoyLoadShader(state, test->sound.shader, sound_resources[0], sound_resources[1], sound_resources[2], sound_resources[3], SHADERTOY_SOUND_PASS);
        const int double_buffering = 2;
        outputs->sound_data_param = init_audio_output(2, 44100, 32, SHADERTOY_SOUND_BLOCK_SIZE * sizeof(float) * SHADERTOY_SOUND_AUDIO_CHANNELS * double_buffering);
    }

    test->start_time = time_now();
}

static bool GetTest(void* data, int idx, const char** out_text) {
    ShadertoyTestInfo *info = (ShadertoyTestInfo*)data;

    *out_text = info->test[idx].title;

    return true;
}

bool GUITestSelection(bool show_selection, ShadertoyTestInfo *info, ShadertoyTest **test) {
    if (!ImGui::Begin("GUI Test selection", &show_selection, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::End();
        return false;
    }

    ImGui::Text("Shadertoy test selection");
    ImGui::Separator();

    static int current_test_id = -1;

    int new_test_id = current_test_id;
    ImGui::ListBox("", &current_test_id, GetTest, info, info->count, 10);

    if (current_test_id != new_test_id) {
        *test = &info->test[current_test_id];
        ImGui::End();
        return true;
    }
    ImGui::End();

    return false;
}

void GUITestOverlay(bool show_overlay, ShadertoyTest *test, ShadertoyState *state, ShadertoyInputs *inputs) {
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    if (!ImGui::Begin("GUI Overlay", &show_overlay, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImGuiIO &io = ImGui::GetIO();
    ImGui::Text("%s (FPS: %.2f)", test->title, io.Framerate);
    ImGui::Separator();
    ImGui::Text("Resolution     x:%d y:%d", (int)inputs->resolution.x, (int)inputs->resolution.y);
    ImGui::Text("Sample rate    %dHz", state->sound_sample_rate);
    ImGui::Text("Date           %d/%d/%d - %ds", (int)inputs->date.x, (int)inputs->date.y, (int)inputs->date.z, (int)inputs->date.w);
    ImGui::Text("Mouse          x:%d y:%d", (int)inputs->mouse.x, (int)inputs->mouse.y);
    ImGui::Text("Time           %.3fs", inputs->global_time);

    ShadertoyResource *image_channels = state->channels[SHADERTOY_IMAGE_PASS];
    ImGui::Separator();
    for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
        // TODO(jose): Show where music is at by having some extra textures
        ImGui::Image((void*)image_channels[i].id, ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128)); ImGui::SameLine();
        if (ImGui::IsItemHovered())
        {
            switch (image_channels[i].type) {
            case SHADERTOY_RESOURCE_TEXTURE:
            case SHADERTOY_RESOURCE_CUBE_MAP:
                ImGui::BeginTooltip();
                ImGui::Text("Filepath       %s", image_files[i][SHADERTOY_IMAGE_PASS].filepath);
                ImGui::Text("Resolution     x:%d y:%d", (int)image_channels[i].resolution.x, (int)image_channels[i].resolution.y);
                ImGui::EndTooltip();
                break;
            case SHADERTOY_RESOURCE_MUSIC: {
                AudioFile *audio = &audio_files[i];
                ImGui::BeginTooltip();
                ImGui::Text("Filepath       %s", audio->filepath);
                ImGui::Text("Resolution     x:%d y:%d", (int)image_channels[i].resolution.x, (int)image_channels[i].resolution.y);
                ImGui::Text("Total samples  %d", audio->samples_count / audio->channels);
                ImGui::Text("Played samples %d", inputs->audio_played_samples[i]);
                ImGui::EndTooltip();
                break;
            }
            default:
                break;
            }
        }
    }
    ImGui::Text("Image pass");

    if (state->sound_enable) {
        ShadertoyResource *sound_channel = state->channels[SHADERTOY_SOUND_PASS];
        ImGui::Separator();
        ImGui::Text("Played samples         %d", inputs->sound_played_samples);
        ImGui::Text("Processed samples      %d", state->sound_process_samples);
        ImGui::Text("Expected samples       %d", state->sound_play_samples);
        for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
            ImGui::Image((void*)sound_channel[i].id, ImVec2(64, 64), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128)); ImGui::SameLine();
            if (ImGui::IsItemHovered())
            {
                if (image_channels[i].type != SHADERTOY_RESOURCE_NONE) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Filepath       %s", image_files[i][SHADERTOY_SOUND_PASS].filepath);
                    ImGui::Text("Resolution     x:%d y:%d", (int)sound_channel[i].resolution.x, (int)sound_channel[i].resolution.y);
                    ImGui::EndTooltip();
                }
            }
        }
        ImGui::Text("Sound pass");
    }


    ImGui::End();
}

// Forward declare as test data contains this
ShadertoyTestInfo* GetTestSuite();
ShadertoyTestInfo* TestInit(TimeFunc time_func, StopAudioFunc stop_audio_func, InitAudioOutputFunc init_audio_output_func, InitAudioInputFunc init_audio_input_func, void *window) {
    time_now = time_func;
    stop_audio = stop_audio_func;
    init_audio_output = init_audio_output_func;
    init_audio_input = init_audio_input_func;
    ImGuiInit(window);
    return GetTestSuite();
}

void TestBegin() {
    ImGuiNewFrame();
}

void TestEnd() {
    ImGui::Render();
}

void TestShutdown() {
    for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
        free(audio_files[i].samples);
    }
    
    ImGuiShutdown();
}

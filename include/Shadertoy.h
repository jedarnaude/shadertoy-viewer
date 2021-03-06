#pragma once

#include <stdint.h>

#include "ShadertoyConfig.h"
// TODO(jose): On the way to remove this implementations from public API.
#include "ShadertoyInternal.h"

#define SHADERTOY_API extern

#ifdef __cplusplus
extern "C" {
#endif

    typedef union {
        struct {
            int x, y;
        };
        int xy[2];
    } ShadertoyIVec2;

    typedef union {
        struct {
            float x, y;
        };
        float xy[2];
    } ShadertoyVec2;

    typedef union {
        struct {
            float x, y, z;
        };
        float xyz[3];
    } ShadertoyVec3;

    typedef union {
        struct {
            float x, y, z, w;
        };
        float xyzw[4];
    } ShadertoyVec4;

    typedef struct {
        int64_t *played_samples;
        float **data;
        int frequency;
        int channel_count;
        int total_samples;
    } ShadertoyAudio;

    /**
    * @warning	Must be zero initialized.
    */
    typedef struct {
        unsigned id;
        int type;
        int size;
        int channel;
        ShadertoyVec3 resolution;
        // TODO(jose): Think a bit more where to store this info
        ShadertoyAudio audio;
    } ShadertoyResource;

	typedef struct {
        uint8_t state[SHADERTOY_KEYMAPS_COUNT];
        uint8_t toggle[SHADERTOY_KEYMAPS_COUNT];
	} ShadertoyKeyboard;

	typedef struct {
        ShadertoyIVec2 min;
        ShadertoyIVec2 max;
	} ShadertoyView;

	typedef enum {
        SHADERTOY_RESOURCE_NONE = 0,
        SHADERTOY_RESOURCE_TEXTURE = 1 << 0,
        SHADERTOY_RESOURCE_CUBE_MAP = 1 << 1,
        SHADERTOY_RESOURCE_KEYBOARD = 1 << 2,
        SHADERTOY_RESOURCE_MUSIC = 1 << 3,
        SHADERTOY_RESOURCE_MICROPHONE = 1 << 4,
    } ResourceType;

	typedef enum {
		SHADERTOY_TEXTURE_FORMAT_RGBA,
		SHADERTOY_TEXTURE_FORMAT_RGB,
        SHADERTOY_TEXTURE_FORMAT_LUMINANCE
	} TextureFormat;

    typedef enum {
        SHADERTOY_TEXTURE_FILTER_LINEAR,
        SHADERTOY_TEXTURE_FILTER_NEAREST,
    } TextureFilter;

	typedef struct {
		TextureFormat format;
        TextureFilter filter;
		int width;
		int height;
		unsigned char* data;
	} Texture;

	typedef enum {
		kFailure,
		kSuccess,
	} Status;

    typedef enum {
        SHADERTOY_IMAGE_PASS,
        SHADERTOY_SOUND_PASS,
        SHADERTOY_PASSES_COUNT
    } ShadertoyPass;

    /**
    * @todo	explain IEEE PCM sample setup
    */
    typedef struct {
        float left;
        float right;
    } ShadertoyAudioSample;

    /**
	 * @todo	explain config options
	 */
	typedef struct {

        // TODO(jose): config rendering stuff (FBO output)

        // Config for sound buffers
        int sound_frequency;
        int sound_playtime;
        int sound_audio_channels;
    } ShadertoyConfig;
    
    /**
    * @todo    explain input structure
    */
    typedef struct {
        ShadertoyKeyboard keyboard;

        ShadertoyVec4 date;
        ShadertoyVec4 mouse;
        ShadertoyVec3 resolution;
        float global_time;

        // Sound related inputs
        int64_t sound_played_samples;

        // Audio sampled time
        int64_t audio_played_samples[SHADERTOY_MAX_CHANNELS];
        
        // Microphone
        int micro_enabled;
        void *micro_data_param;   // Pointer to platform specific data identifier for audio if needed
        ShadertoyAudioSample *micro_samples;
    } ShadertoyInputs;

    /**
     * @todo    explain output structure
     */
    typedef struct {
        ShadertoyAudioSample *sound_next_buffer;
        int sound_buffer_size;
        void *sound_data_param;   // Pointer to platform specific data identifier for audio if needed
        int sound_should_stop;
        int sound_enabled;

        void *music_data_param[SHADERTOY_MAX_CHANNELS];
        int music_enabled_channel[SHADERTOY_MAX_CHANNELS];
        int music_enabled;
    } ShadertoyOutputs;    

    /**
    * @todo	explain what is a shadertoy state.
    * keyboard = limited to one
    * music = any channel
    */
    typedef struct {
        // General stuff
        ShadertoyShader shader[SHADERTOY_PASSES_COUNT];
        ShadertoyResource channels[SHADERTOY_PASSES_COUNT][SHADERTOY_MAX_CHANNELS];

        // Render specifics
        int texture_enable_2d;
        ShadertoyResource image_fbo;

        // Keyboard
        int keyboard_enable;
        int keyboard_enabled_channel;

        // Audio
        // TODO(jose): rename music to audio
        int music_enabled;
        int music_enabled_channel[SHADERTOY_MAX_CHANNELS];

        // Sound
        int sound_enable;
        int sound_frequency;
        int sound_sample_rate;
        int sound_play_time;
        int sound_play_samples;
        int sound_audio_channels;
        int sound_process_samples;
        Texture sound_texture;
        ShadertoyResource sound_resource;
        ShadertoyResource sound_fbo;
        ShadertoyAudioSample *sound_buffers[SHADERTOY_SOUND_BUFFERS_COUNT];
        
        // Microphone
        int micro_enabled;
        int micro_enabled_channel;

        // Output link
        // TODO(jose): Is this a good internal dependency?
        ShadertoyInputs* inputs;
        ShadertoyOutputs* outputs;
    } ShadertoyState;

	/**
	 * Init a ShadertoyState.
	 * @see	ShadertoyState
	 * @param[in]	config	configuration used for setting up data for our ShadertoyState.
	 * @param[out]	state	shadertoy state containing useful information for our rendering phase.
     * @param[out]	outputs	shadertoy outputs containing already initialized buffers for outputs to be processed.
     */
    SHADERTOY_API Status ShadertoyInit(ShadertoyConfig *config, ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs);
	
	/**
	 * Function responsible for rendering a shadertoy state.
	 * @todo	explain what is a shadertoy state (image,sound,vr?)
	 */
    SHADERTOY_API Status ShadertoyRender(ShadertoyState *state, ShadertoyInputs *in, ShadertoyView *view, ShadertoyOutputs *out);

    /**
    * Free a ShadertoyState.
    * This function is mean to free any allocated data from a ShadertoyState.
    * @see	ShadertoyState
    * @param[in]	state	shadertoy state that allows understanding of resources that should be freed.
    * @param[in]	outputs	audio buffers are freed if needed.
    */
    SHADERTOY_API Status ShadertoyFree(ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs);

    /**
     * Generates correct shader based on shadertoy input source code, will 
     * compiles and link shader based on resource descriptions.
     * Please note that resource types are needed for correct code generation
     * but there is no need for resources themselves to be loaded.
     *
     * @param[in]   state       shadertoy state used to embed this shader pass.
     * @param[in]   source      shadertoy source code.
     * @param[in]   channels    resoure type for each channel, used to generate shader source code.
     * @param[in]   pass        shader pass this source is related to.
     */
    SHADERTOY_API Status ShadertoyLoadShader(ShadertoyState *state, const char *source, ResourceType channel0, ResourceType channel1, ResourceType channel2, ResourceType channel3, ShadertoyPass pass);

    /**
     * Loads texture information into a shadertoy channel.
     */
    SHADERTOY_API Status ShadertoyLoadTexture(ShadertoyState *state, Texture *in, ShadertoyPass pass, int channel_id);

    /**
     * Loads cubemap information into a shadertoy channel.
     */
    SHADERTOY_API Status ShadertoyLoadCubemap(ShadertoyState *state, Texture *in, ShadertoyPass pass, int channel_id);

    /**
     * Loads audio information into a shadertoy channel.
     */
    SHADERTOY_API Status ShadertoyLoadAudio(ShadertoyState *state, ShadertoyAudio *in, ShadertoyPass pass, int channel_id);

#ifdef __cplusplus
}
#endif

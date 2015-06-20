#include "Shadertoy.h"
#include "ShadertoyInternal.h"

#include "opengl_internal.h"
#include "kiss_fft.h"

// System headers
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <limits.h>

// Helper headers
#include "MathHelpers.h"


#define FFT_SIZE 2048
static kiss_fft_cpx fft_in[FFT_SIZE];
static kiss_fft_cpx fft_out[FFT_SIZE];

static GLubyte sound_readpixels_buffer[SHADERTOY_SOUND_BLOCK_SIZE * 4];

static const char vertex_source[] = R"(
	attribute vec2 vPosition;
	void main() {
		gl_Position = vec4(vPosition, 0.0, 1.0);
	}
)";

static ShadertoyShaderEmbed embed_shader[SHADERTOY_PASSES_COUNT] =
{
    {   // Image pass
        // Prefix
        R"(
            #extension GL_OES_standard_derivatives : enable

            uniform vec3 iResolution;
	        uniform float iGlobalTime;
	        uniform float iChannelTime[4];
	        uniform vec3 iChannelResolution[4];
	        uniform vec4 iMouse;
	        uniform %s iChannel0;
	        uniform %s iChannel1;
	        uniform %s iChannel2;
	        uniform %s iChannel3;
	        uniform vec4 iDate;
	        uniform float iSampleRate;
        )",
        // Sufix
        R"(

        	void main( void )
	        {
        		vec4 color = vec4(0.0,0.0,0.0,1.0);
		        mainImage( color, gl_FragCoord.xy );
		        color.w = 1.0;
		        gl_FragColor = color;
	        }
        )"
    },
    {   // Sound pass
        // Prefix
        R"(
            #extension GL_OES_standard_derivatives : enable

    	    uniform float iChannelTime[4];
	        uniform vec3 iChannelResolution[4];
	        uniform float iBlockOffset;
	        uniform %s iChannel0;
	        uniform %s iChannel1;
	        uniform %s iChannel2;
	        uniform %s iChannel3;
	        uniform vec4 iDate;
	        uniform float iSampleRate;
        )",
        // Sufix
        R"(
            void main()
            {
                float t = iBlockOffset + (gl_FragCoord.x + gl_FragCoord.y*512.0)/44100.0;
        
                vec2 y = mainSound( t );
        
                vec2 v  = floor((0.5+0.5*y)*65536.0);
                vec2 vl = mod(v,256.0)/255.0;
                vec2 vh = floor(v/256.0)/255.0;
        
                gl_FragColor = vec4(vl.x,vh.x,vl.y,vh.y);
            }
        )"
    }
};

// TODO(jose): Let´s remove this class, I am not completely convinced of using it.
/**
 * Buffer class used in a RAII way.
 * In order to avoid allocation around our code we needed a buffer class. As
 * this project aims to be as simple as possible I´ve decided to remove any 
 * STL dependencies, thus no String or unique_ptr can be used.
 * This leaves us with some "not so nice" new/delete duo. This class aim to
 * control them by just generating a non copiable buffer.
 * @warning this buffer class always zero initializes the buffer.
 */
struct ShadertoyBuffer {
    ShadertoyBuffer() : data(NULL) {
    }

    explicit ShadertoyBuffer(int size) : data(NULL) {
        Reset(size);
    }

    void Reset(int size) {
        delete[] data;
        data = new char[size] {};
    }

    ~ShadertoyBuffer() {
        delete[] data;
    }

    char *data;

private:
    // Avoid unwanted automatic code generation
    ShadertoyBuffer(const ShadertoyBuffer& obj);
    ShadertoyBuffer& operator=(const ShadertoyBuffer& obj);
};

static GLfloat vertices[] = {
	-1.f, -1.f,
	-1.f,  1.f,
	1.f,  1.f,
	1.f, -1.f,
};

static GLubyte indices[] = {
	0, 1, 2,
	0, 2, 3
};

/**
 * ShadertoyUniform upload code for vec3.
 */
void UploadUniform(ShadertoyUniform& uniform, ShadertoyVec3& value) {
    glUniform3fv(uniform.location, 1, value.xyz);
}

/**
 * ShadertoyUniform upload code for vec4.
 */
void UploadUniform(ShadertoyUniform& uniform, ShadertoyVec4& value) {
    glUniform4fv(uniform.location, 1, value.xyzw);
}

/**
 * ShadertoyUniform upload code for float.
 */
void UploadUniform(ShadertoyUniform& uniform, float value) {
    glUniform1f(uniform.location, value);
}

/**
 * ShadertoyUniform upload code for int.
 */
void UploadUniform(ShadertoyUniform& uniform, int value) {
    glUniform1i(uniform.location, value);
}

static const char*
GetShaderType(GLint type) {
    switch (type) {
    case GL_VERTEX_SHADER:    return "vertex";
    case GL_FRAGMENT_SHADER:  return "fragment";
    }
    return "unknown";
}

/**
 * Compiles a source shader based on requested type.
 *
 * @param[in]	source	shader source code.
 * @param[in]	type	shader type (either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
 * @return	if succesful will return GL id, otherwise will return 0 and print error code on default output.
 */
static GLuint
CompileShader(const char* source, GLint type) {
	// TODO(jose): printf should not be used, we need a default output
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint log_lenght;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_lenght);
	if (log_lenght > 1) {
        ShadertoyBuffer buffer(log_lenght);
		glGetShaderInfoLog(shader, log_lenght, NULL, buffer.data);
		printf("LOG %s: \n %s", GetShaderType(type), buffer.data);
	}

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

/**
 * Generates a program with valid vertex and fragment source code.
 *
 * @param[in]	vert_source		vertex source code.
 * @param[in]	frag_source		fragment source code.
 * @return	if succesful will return GL id, otherwise will return 0 and print error code on default output.
 */
static GLuint
CreateProgram(const char* vert_source, const char* frag_source) {
	GLint vert_shader = CompileShader(vert_source, GL_VERTEX_SHADER);
	GLint frag_shader = CompileShader(frag_source, GL_FRAGMENT_SHADER);

	GLuint program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);

	GLint log_lenght;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_lenght);
	if (log_lenght > 1) {
        ShadertoyBuffer buffer(log_lenght);
        glGetProgramInfoLog(program, log_lenght, nullptr, buffer.data);
		printf("LOG link: \n %s", buffer.data);
	}

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		if (vert_shader)	glDeleteShader(vert_shader);
		if (frag_shader)	glDeleteShader(frag_shader);
		if (program)		glDeleteProgram(program);
		return 0;
	}

	return program;
}

/**
* Generates correct name for shader code based on resource type.
*
* @param[in]	type	resource type.
* @return	valid sampler type name, sampler2D is default.
*/
static const char*
GetShaderUniformType(ResourceType type) {
	return type == SHADERTOY_RESOURCE_CUBE_MAP ? "samplerCube" : "sampler2D";
}

/**
 * This method does some basic string concatenation on shader sources.
 *
 * @param[in]   prefix      shader prefix based on layer type.
 * @param[in]   source      user source code.
 * @param[in]   suffix      shader suffix based on layer type.
 * @param[in]   channels    resource types based on channels (mainly for sample string generation).
 * @param[in]   buffer      buffer were we will store a valid shader to be compiled.
 */
static void
GetShaderSource(const char *prefix, const char *source, const char *suffix, ResourceType channels[SHADERTOY_MAX_CHANNELS], ShadertoyBuffer* buffer) {
    size_t prefix_size = strlen(prefix);
    size_t source_size = strlen(source);
    size_t suffix_size = strlen(suffix);

    // Generate correct prefix
    ShadertoyBuffer final_prefix((int)prefix_size * 2);
    sprintf(final_prefix.data, prefix, GetShaderUniformType(channels[0]), GetShaderUniformType(channels[1]), GetShaderUniformType(channels[2]), GetShaderUniformType(channels[3]));
    size_t final_prefix_size = strlen(final_prefix.data);

    // Copy strings
    buffer->Reset((int)(final_prefix_size + source_size + suffix_size + 1));
    strcpy(buffer->data, final_prefix.data);
    strcpy(buffer->data + final_prefix_size, source);
    strcpy(buffer->data + final_prefix_size + source_size, suffix);
}

/**
 * Create a shadertoy shader based on user input. Queries all uniforms for 
 * runtime usage.
 *
 * @param[in]   source		user source code.
 * @param[out]	shader		shadertoy shader type ready to be used.
 */
static Status
CreateShader(const char *source, ShadertoyShader *shader) {
	memset(shader, 0, sizeof(*shader));

	GLuint program = CreateProgram(vertex_source, source);

	if (program == 0) {
		return kFailure;
	}

	// Locate uniforms
	shader->program = program;
	shader->date = { glGetUniformLocation(program, "iDate"), SHADERTOY_UNIFORM_VEC4 };
	shader->mouse = { glGetUniformLocation(program, "iMouse"), SHADERTOY_UNIFORM_VEC4};
	shader->resolution = { glGetUniformLocation(program, "iResolution"), SHADERTOY_UNIFORM_VEC3 };
	shader->sample_rate = { glGetUniformLocation(program, "iSampleRate"), SHADERTOY_UNIFORM_FLOAT };
	shader->global_time = { glGetUniformLocation(program, "iGlobalTime"), SHADERTOY_UNIFORM_FLOAT };
    shader->block_offset = { glGetUniformLocation(program, "iBlockOffset"), SHADERTOY_UNIFORM_FLOAT };

    for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
		char const *channel = NULL;
		char const *resolution = NULL;
		char const *time = NULL;
		switch (i) {
		case 0:	channel = "iChannel0";	resolution = "iChannelResolution[0]"; time = "iChannelTime[0]"; break;
		case 1:	channel = "iChannel1";	resolution = "iChannelResolution[1]"; time = "iChannelTime[1]"; break;
		case 2:	channel = "iChannel2";	resolution = "iChannelResolution[2]"; time = "iChannelTime[2]"; break;
		case 3:	channel = "iChannel3";	resolution = "iChannelResolution[3]"; time = "iChannelTime[3]"; break;
		default:
			assert(false);
		}
		shader->channels[i].sample = { glGetUniformLocation(program, channel), SHADERTOY_UNIFORM_SAMPLER };
		shader->channels[i].resolution = { glGetUniformLocation(program, resolution), SHADERTOY_UNIFORM_VEC3 };
		shader->channels[i].time = { glGetUniformLocation(program, time), SHADERTOY_UNIFORM_FLOAT };
	}

	return kSuccess;
}

/**
 * Create a framebuffer object based on texture requirements.
 *
 * @param[in]   texture		    texture configuration requested for FBO.
 * @param[out]	framebuffer     framebuffer as a resource to be used.
 */
static Status
CreateFramebufferObject(ShadertoyResource *texture, ShadertoyResource *framebuffer) {
    glGenFramebuffers(1, &framebuffer->id);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->id, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        return kFailure;
    }

    return kSuccess;
}

/**
 * Texture blitting code, will decide size of the blitting based on resource information.
 *
 * @param[in]   resource    resource of texture type where blitting will be aim to.
 * @param[out]  data        data we need to blit.
 */
static void
BlitToTexture(ShadertoyResource *resource, GLvoid  *data) {
    GLenum type = resource->size == 4 ? GL_RGBA : resource->size == 3 ? GL_RGB : GL_RED;
    glTexImage2D(GL_TEXTURE_2D,
        0,
        type,
        (GLsizei)resource->resolution.x,
        (GLsizei)resource->resolution.y,
        0,
        type,
        GL_UNSIGNED_BYTE,
        data);
}

/**
 * Process the input signals and converts it to byte output for texture. Most 
 * of this implementation follows Chromium open source project. Thus constants
 * always equals to default values used there.
 *
 * @param[in]	pcm		IEEE PCM input buffer to work with
 * @param[out]	out		byte buffer of size 1024 where to write the output from our processing.
 * @see	https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/modules/webaudio/RealtimeAnalyser.cpp
 */
static void
ProcessAudio(ShadertoyAudio *audio, uint8_t *out) {
	// Location within PCM buffer
	float *pcm_location = *audio->data + (*audio->played_samples * audio->channel_count);

	// Add and scale input samples
	float samples[FFT_SIZE] = {};
    int missing_samples = audio->total_samples - (int)*audio->played_samples;
    int fft_size = missing_samples > FFT_SIZE ? FFT_SIZE : missing_samples;
	for (int i = 0; i < fft_size; ++i) {
		samples[i] = (pcm_location[i * 2] + pcm_location[(i * 2) + 1]) * 0.5f;
	}

	for (int i = 0; i < FFT_SIZE; ++i) {
		fft_in[i].r = samples[i];
		fft_in[i].i = 0;
	}

	// Window the input samples (Blackman window)
	double alpha = 0.16;
	double a0 = 0.5 * (1 - alpha);
	double a1 = 0.5;
	double a2 = 0.5 * alpha;

	for (int i = 0; i < FFT_SIZE; ++i) {
		double x = static_cast<double>(i) / static_cast<double>(FFT_SIZE);
		double window = a0 - a1 * cos(2.0 * M_PI * x) + a2 * cos(2.0 * M_PI * 2.0 * x);
		fft_in[i].r *= float(window);
	}

	// Apply FFT
	kiss_fft_cfg fft_cfg = kiss_fft_alloc(FFT_SIZE, 0, 0, 0);
	kiss_fft(fft_cfg, fft_in, fft_out);
	kiss_fft_free(fft_cfg);

	// Blow away the packed nyquist component.
	fft_out[0].i = 0;

	// Normalize so than an input sine wave at 0dBfs registers as 0dBfs (undo FFT scaling factor).
	const double modulo_scale = 1.0 / FFT_SIZE;

	// A value of 0 does no averaging with the previous result.  Larger values produce slower, but smoother changes.
	double k = SHADERTOY_AUDIO_SMOOTHING_CONSTANT;
	k = Max(0.0, k);
	k = Min(1.0, k);

	// Convert the analysis data from complex to magnitude and average with the previous result.
	static float magnitudes[FFT_SIZE / 2] = {};
	for (size_t i = 0; i < FFT_SIZE / 2; ++i) {
		double modulus = sqrt(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);
		double scalar_magnitude = modulus * modulo_scale;
		magnitudes[i] = float(k * magnitudes[i] + (1 - k) * scalar_magnitude);
	}

	// Spectrum
	// Convert from linear magnitude to unsigned-byte decibels.
	const double rangeScaleFactor = 1.0 / (SHADERTOY_AUDIO_MAX_DECIBELS - SHADERTOY_AUDIO_MIN_DECIBELS);
	const double minDecibels = SHADERTOY_AUDIO_MIN_DECIBELS;

	for (unsigned i = 0; i < FFT_SIZE / 2; ++i) {
		float linearValue = magnitudes[i];
		double dbMag = !linearValue ? minDecibels : LinearToDecibels(linearValue);

		// The range m_minDecibels to m_maxDecibels will be scaled to byte values from 0 to UCHAR_MAX.
		double scaledValue = UCHAR_MAX * (dbMag - minDecibels) * rangeScaleFactor;

		out[i] = (uint8_t)Clamp(scaledValue, 0, UCHAR_MAX);
	}

	// Waveform
	// TODO(jose): review seems not correct
    int end_waveform = 512 + (fft_size / 4);
	for (int i = 512; i < end_waveform; ++i) {
		// Buffer access is protected due to modulo operation.
		float value = pcm_location[i - 512];

		// Scale from nominal -1 -> +1 to unsigned byte.
		double scaledValue = 128 * (value + 1);

		out[i] = (uint8_t)Clamp(scaledValue, 0, UCHAR_MAX);
	}
}

static void
ProcessSound(int block_offset, int sample_rate, ShadertoyShader *shader, ShadertoyAudioSample *samples) {
    // Process audio on GL
    UploadUniform(shader->block_offset, (float)block_offset / sample_rate);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
    glReadPixels(0, 0, SHADERTOY_SOUND_TEXTURE_WIDTH, SHADERTOY_SOUND_TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, sound_readpixels_buffer);

    // Copy to output
    for (int j = 0; j < SHADERTOY_SOUND_BLOCK_SIZE; ++j) {
        samples[j].left = -1.0f + 2.0f*(sound_readpixels_buffer[4 * j + 0] + 256.0f*sound_readpixels_buffer[4 * j + 1]) / 65535.0f;
        samples[j].right = -1.0f + 2.0f*(sound_readpixels_buffer[4 * j + 2] + 256.0f*sound_readpixels_buffer[4 * j + 3]) / 65535.0f;
    }
}

/**
 * Gets the first channel that has a keyboard type event. Shadertoy does not
 * support multiple keyboards at the moment.
 * @param[in]   channels    shadertoy channels to be scan
 * @return  returns the first channel using keyboard, 0 if not found. 
 * @warning invalid channel might be returneded if there is no keyboard 
 */
static int
GetKeyboardChannel(ResourceType channels[SHADERTOY_MAX_CHANNELS]) {
    for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
        if (channels[i] == SHADERTOY_RESOURCE_KEYBOARD) {
            return i;
        }
    }

    return 0;
}

static int
NeedsResourceType(ResourceType channels[SHADERTOY_MAX_CHANNELS], ResourceType type) {
    return channels[0] & type || channels[1] & type || channels[2] & type || channels[3] & type;
}

static void
RenderImage(ShadertoyState *state, ShadertoyInputs *in, ShadertoyView *view) {
    ShadertoyShader *shader = &state->shader[SHADERTOY_IMAGE_PASS];
    ShadertoyResource *channels = state->channels[SHADERTOY_IMAGE_PASS];

    glBindFramebuffer(GL_FRAMEBUFFER, state->image_fbo.id);
    glViewport(view->min.x, view->min.y, (view->max.x - view->min.x), (view->max.y - view->min.y));

    glUseProgram(shader->program);

	UploadUniform(shader->date, in->date);
	UploadUniform(shader->mouse, in->mouse);
	UploadUniform(shader->resolution, in->resolution);
	UploadUniform(shader->sample_rate, state->sound_sample_rate);
	UploadUniform(shader->global_time, in->global_time);

    if (state->keyboard_enable) {
        ShadertoyResource* keyboard_resource = &channels[state->keyboard_enabled_channel];
        BlitToTexture(keyboard_resource,  in->keyboard.state);
    }

	glEnable(GL_TEXTURE_2D);
	uint8_t texture_data[1024];
	for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
        if (state->music_enabled_channel[i]) {
            ShadertoyAudio *audio = &channels[i].audio;
			UploadUniform(shader->channels[i].time, (float)*audio->played_samples / audio->frequency);
			ProcessAudio(audio, texture_data);
			BlitToTexture(&channels[i], texture_data);
        }

        ShadertoyResource* res = &channels[i];
		if (res->id) {
			GLenum texture_target = GL_TEXTURE_2D;
			switch (res->type)
			{
			case SHADERTOY_RESOURCE_CUBE_MAP:
				texture_target = GL_TEXTURE_CUBE_MAP;
			case SHADERTOY_RESOURCE_TEXTURE:
            case SHADERTOY_RESOURCE_KEYBOARD:
            case SHADERTOY_RESOURCE_MUSIC:
                glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(texture_target, res->id);

				UploadUniform(shader->channels[i].sample, i);
				UploadUniform(shader->channels[i].resolution, res->resolution);
				break;
			default:
				break;
			}
		}
	}
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
}

static void
RenderSound(ShadertoyState *state, ShadertoyInputs *in, ShadertoyOutputs *out) {
    // Audio is over no need to process more
    if (state->sound_process_samples > state->sound_play_samples) {
        out->sound_next_buffer = NULL;
        out->sound_buffer_size = 0;
        return;
    }

    int current_process_block = state->sound_process_samples / SHADERTOY_SOUND_BLOCK_SIZE;
    int next_play_block = ((int)in->sound_played_samples + SHADERTOY_SOUND_BLOCK_SIZE - 1) / SHADERTOY_SOUND_BLOCK_SIZE;  // ceil

    // We have enough buffered sound, no need to process more
    if (current_process_block >= (next_play_block + 1)) {
        out->sound_next_buffer = NULL;
        out->sound_buffer_size = 0;
        return;
    }
     
    // Proceed to process more sound 
    ShadertoyShader* shader = &state->shader[SHADERTOY_SOUND_PASS];
    ShadertoyResource *channels = state->channels[SHADERTOY_SOUND_PASS];
    Texture *texture = &state->sound_texture;

    glBindFramebuffer(GL_FRAMEBUFFER, state->sound_fbo.id);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, texture->width, texture->height);
    glUseProgram(shader->program);

    UploadUniform(shader->date, in->date);
    UploadUniform(shader->sample_rate, state->sound_sample_rate);
    
    glEnable(GL_TEXTURE_2D);
    for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
        ShadertoyResource* res = &channels[i];
        if (res->id) {
            GLenum texture_target = GL_TEXTURE_2D;
            switch (res->type)
            {
            case SHADERTOY_RESOURCE_CUBE_MAP:
                texture_target = GL_TEXTURE_CUBE_MAP;
            case SHADERTOY_RESOURCE_TEXTURE:
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(texture_target, res->id);

                UploadUniform(shader->channels[i].sample, i);
                UploadUniform(shader->channels[i].resolution, res->resolution);
                break;
            default:
                // TODO(jose): final API should report error or some feedback here I guess
                break;
            }
        }
    }

    // Process sound on demand
    int block_offset = current_process_block * SHADERTOY_SOUND_BLOCK_SIZE;
    ShadertoyAudioSample *current_buffer = state->sound_buffers[current_process_block % SHADERTOY_SOUND_BUFFERS_COUNT];
    ProcessSound(block_offset, state->sound_sample_rate, shader, current_buffer);

    state->sound_process_samples += SHADERTOY_SOUND_BLOCK_SIZE;

    int buffer_size = SHADERTOY_SOUND_BLOCK_SIZE * sizeof(ShadertoyAudioSample);
    int samples_diff = state->sound_play_samples - state->sound_process_samples;
    if (samples_diff < 0) {
        buffer_size = (SHADERTOY_SOUND_BLOCK_SIZE + samples_diff) * sizeof(ShadertoyAudioSample);
    }

    // Check if sound shall stop defined by our settings
    out->sound_buffer_size = buffer_size;
    out->sound_next_buffer = current_buffer;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static Status
ShadertoyLoadTexture(ShadertoyState *state, Texture *in, ShadertoyResource *out) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    GLint filtering = in->filter == SHADERTOY_TEXTURE_FILTER_LINEAR ? GL_LINEAR : GL_NEAREST;
    GLint internal_format = 0;
    GLenum format = 0;
    int size = 0;
    switch (in->format) {
    case SHADERTOY_TEXTURE_FORMAT_RGBA:
        internal_format = format = GL_RGBA;
        size = 4;
        break;
    case SHADERTOY_TEXTURE_FORMAT_RGB:
        internal_format = format = GL_RGB;
        size = 3;
        break;
    case SHADERTOY_TEXTURE_FORMAT_LUMINANCE:
        internal_format = format = GL_RED;
        size = 1;
        break;
    default:
        // TODO(jose): Missing error reporting 
        assert(false);
        break;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, in->width, in->height, 0, format, GL_UNSIGNED_BYTE, in->data);

    out->type = SHADERTOY_RESOURCE_TEXTURE;
    out->id = texture_id;
    out->size = size;
    out->resolution.x = (float)in->width;
    out->resolution.y = (float)in->height;
    out->resolution.z = 1.0f;

    return kSuccess;
}

//***************************************************************************//
// API public functions implementations										 //
//***************************************************************************//
Status
ShadertoyInit(ShadertoyConfig *config, ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs) {
	Status result = kSuccess;

    state->sound_frequency = config->sound_frequency != 0 ? config->sound_frequency : SHADERTOY_SOUND_FREQUENCY;
    state->sound_play_time = config->sound_playtime != 0 ? config->sound_playtime : SHADERTOY_SOUND_PLAYTIME;
    state->sound_audio_channels = config->sound_audio_channels != 0 ? config->sound_audio_channels : SHADERTOY_SOUND_AUDIO_CHANNELS;

    state->outputs = outputs;

    // TODO(jose): OpenGL extensions specifics
    state->image_fbo.id = 0;
    
    // Clear color
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	return result;
}

Status 
ShadertoyFree(ShadertoyState *state, ShadertoyInputs *inputs, ShadertoyOutputs *outputs) {
    // TODO(jose): gl delete any resources that we did allocate.
    for (int j = 0; j < SHADERTOY_PASSES_COUNT; ++j) {
        for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
            if (state->channels[j][i].type == SHADERTOY_RESOURCE_CUBE_MAP || state->channels[j][i].type == SHADERTOY_RESOURCE_TEXTURE) {
                glDeleteTextures(1, &state->channels[j][i].id);
            }
        }
    }

    if (state->sound_enable) {
        delete[] state->sound_buffers[0];
    }

    *state = {};
    *inputs = {};
    *outputs = {};

    return kSuccess;
}

Status
ShadertoyRender(ShadertoyState *state, ShadertoyInputs *in, ShadertoyView *view, ShadertoyOutputs *out) {
    // Enable vertex attribs (redundant but this way we are "stateless")
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, vertices);

    RenderImage(state, in, view);
    if (state->sound_enable) {
        RenderSound(state, in, out);

        // TODO(jose): the >= is not nice, we should figure out a way to do it exactly.
        // Inform that sound should stop
        if (!out->sound_should_stop && (in->sound_played_samples >= state->sound_play_samples)) {
            out->sound_should_stop = 1;
        }
    }

    // Prepare special outputs
    out->music_enabled = state->music_enabled;
    out->sound_enabled = state->sound_enable;
    memcpy(out->music_enabled_channel, state->music_enabled_channel, sizeof(out->music_enabled_channel));

    glDisableVertexAttribArray(0);
    
    return kSuccess;
}

Status 
ShadertoyLoadShader(ShadertoyState *state, const char *source, ResourceType channel0, ResourceType channel1, ResourceType channel2, ResourceType channel3, ShadertoyPass pass) {
    ShadertoyShader *shader = &state->shader[pass];
    ShadertoyShaderEmbed *embed = &embed_shader[pass];
    ResourceType channels[SHADERTOY_MAX_CHANNELS] = { channel0, channel1, channel2, channel3 };

    ShadertoyBuffer final_image_shader;
    GetShaderSource(embed->prefix, source, embed->sufix, channels, &final_image_shader);
    CreateShader(final_image_shader.data, shader);

    // Load state for shader
    if (pass == SHADERTOY_IMAGE_PASS) {
        int need_keyboard = NeedsResourceType(channels, SHADERTOY_RESOURCE_KEYBOARD);
        int need_music = NeedsResourceType(channels, SHADERTOY_RESOURCE_MUSIC);

        state->texture_enable_2d = NeedsResourceType(channels, SHADERTOY_RESOURCE_TEXTURE);
        state->texture_enable_2d |= NeedsResourceType(channels, SHADERTOY_RESOURCE_CUBE_MAP);
        state->texture_enable_2d |= need_keyboard;

        // Sample rate is a fixed data
        state->sound_sample_rate = state->sound_frequency;

        // Special inputs
        if (need_keyboard) {
            state->keyboard_enable = 1;
            state->keyboard_enabled_channel = GetKeyboardChannel(channels);

            Texture texture = { SHADERTOY_TEXTURE_FORMAT_LUMINANCE, SHADERTOY_TEXTURE_FILTER_NEAREST , SHADERTOY_KEYMAPS_COUNT, 2, NULL };
            ShadertoyLoadTexture(state, &texture, pass, state->keyboard_enabled_channel);
        }

        if (need_music) {
            state->music_enabled = 1;
            for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
                if (channels[i] == SHADERTOY_RESOURCE_MUSIC) {
                    state->music_enabled_channel[i] = 1;
                }
            }
        }
    }

    if (pass == SHADERTOY_SOUND_PASS) {
        // Block allocation
        ShadertoyAudioSample *buffer = (ShadertoyAudioSample*)malloc(sizeof(ShadertoyAudioSample) * SHADERTOY_SOUND_BLOCK_SIZE * SHADERTOY_SOUND_BUFFERS_COUNT);
        memset(buffer, 0, sizeof(ShadertoyAudioSample) * SHADERTOY_SOUND_BLOCK_SIZE * SHADERTOY_SOUND_BUFFERS_COUNT);

        // Sound configurations, we use shadertoy defaults if not specified 
        int sound_frequency = state->sound_frequency;
        int sound_play_time = state->sound_play_time;

        for (int i = 0; i < SHADERTOY_SOUND_BUFFERS_COUNT; ++i) {
            state->sound_buffers[i] = (ShadertoyAudioSample*)(buffer + i * SHADERTOY_SOUND_BLOCK_SIZE);
        }
        state->sound_enable = 1;
        state->sound_sample_rate = sound_frequency;
        state->sound_play_time = sound_play_time;
        state->sound_play_samples = state->sound_play_time * state->sound_sample_rate;
        state->sound_texture = { SHADERTOY_TEXTURE_FORMAT_RGBA, SHADERTOY_TEXTURE_FILTER_NEAREST, SHADERTOY_SOUND_TEXTURE_WIDTH, SHADERTOY_SOUND_TEXTURE_HEIGHT, NULL };
        ShadertoyLoadTexture(state, &state->sound_texture, &state->sound_resource);

        // On GL ES 2.0 devices that do not support PBO we need to read from framebuffer unfortunately
        CreateFramebufferObject(&state->sound_resource, &state->sound_fbo);
    }

    return kSuccess;
}

Status
ShadertoyLoadTexture(ShadertoyState *state, Texture *in, ShadertoyPass pass, int channel_id) {
    return ShadertoyLoadTexture(state, in, &state->channels[pass][channel_id]);
}

Status
ShadertoyLoadCubemap(ShadertoyState *state, Texture *in, ShadertoyPass pass, int channel_id) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    for (int i = 0; i < 6; ++i) {
        Texture* texture = &in[i];

        GLint internal_format = texture->format == SHADERTOY_TEXTURE_FORMAT_RGBA ? GL_RGBA : GL_RGB;
        GLenum format = texture->format == SHADERTOY_TEXTURE_FORMAT_RGBA ? GL_RGBA : GL_RGB;

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, texture->data);

    }

    ShadertoyResource *resource = &state->channels[pass][channel_id];
    resource->type = SHADERTOY_RESOURCE_CUBE_MAP;
    resource->id = texture_id;
    resource->resolution.x = (float)in->width;
    resource->resolution.y = (float)in->height;
    resource->resolution.z = 1.0f;

    return kSuccess;
}


Status
ShadertoyLoadAudio(ShadertoyState *state, ShadertoyAudio *in, ShadertoyPass pass, int channel_id) {
    Texture texture = { SHADERTOY_TEXTURE_FORMAT_LUMINANCE, SHADERTOY_TEXTURE_FILTER_NEAREST , SHADERTOY_AUDIO_TEXTURE_WIDTH, 2, NULL };
    ShadertoyLoadTexture(state, &texture, pass, channel_id);
    ShadertoyResource *resource = &state->channels[pass][channel_id];
    resource->type = SHADERTOY_RESOURCE_MUSIC;
    resource->audio = *in;

    return kSuccess;
}

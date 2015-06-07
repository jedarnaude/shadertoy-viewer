#pragma once

// TODO(jose): Move internal stuff to "hidden" header.
typedef struct {
    const char *prefix;
    const char *sufix;
} ShadertoyShaderEmbed;

typedef enum {
    SHADERTOY_UNIFORM_VEC3,
    SHADERTOY_UNIFORM_VEC4,
    SHADERTOY_UNIFORM_FLOAT,
    SHADERTOY_UNIFORM_SAMPLER,
} ShadertoyUniformType;

typedef struct {
    int location;
    ShadertoyUniformType type;
} ShadertoyUniform;

typedef struct {
    ShadertoyUniform time;
    ShadertoyUniform resolution;
    ShadertoyUniform sample;
} ShadertoyChannel;

typedef struct {
    unsigned int program;

    ShadertoyUniform resolution;
    ShadertoyUniform global_time;
    ShadertoyUniform date;
    ShadertoyUniform mouse;
    ShadertoyUniform sample_rate;
    ShadertoyUniform block_offset;

    ShadertoyChannel channels[SHADERTOY_MAX_CHANNELS];
} ShadertoyShader;

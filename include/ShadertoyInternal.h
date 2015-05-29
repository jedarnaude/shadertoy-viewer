#pragma once

// TODO(jose): Move internal stuff to "hidden" header.
struct ShadertoyShaderEmbed {
    const char *prefix;
    const char *sufix;
};

enum ShadertoyUniformType {
    SHADERTOY_UNIFORM_VEC3,
    SHADERTOY_UNIFORM_VEC4,
    SHADERTOY_UNIFORM_FLOAT,
    SHADERTOY_UNIFORM_SAMPLER,
};

struct ShadertoyUniform {
    int location;
    ShadertoyUniformType type;
};

struct ShadertoyChannel {
    ShadertoyUniform time;
    ShadertoyUniform resolution;
    ShadertoyUniform sample;
};

struct ShadertoyShader {
    unsigned int program;

    ShadertoyUniform resolution;
    ShadertoyUniform global_time;
    ShadertoyUniform date;
    ShadertoyUniform mouse;
    ShadertoyUniform sample_rate;
    ShadertoyUniform block_offset;

    ShadertoyChannel channels[SHADERTOY_MAX_CHANNELS];
};

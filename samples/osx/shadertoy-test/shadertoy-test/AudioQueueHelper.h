
#ifndef shadertoy_test_AudioQueueHelper_h
#define shadertoy_test_AudioQueueHelper_h

#include <AudioToolbox/AudioQueue.h>

#define AUDIOQUEUE_BUFFERS_COUNT 3

typedef struct {
    AudioStreamBasicDescription format;
    AudioQueueRef queue;
    AudioQueueBufferRef buffers[AUDIOQUEUE_BUFFERS_COUNT];
    int buffer_count;
    bool is_running;
} AudioQueueUnit;


static void AudioQueueFeedOutput(void *custom_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    // We don't feed buffer the normal AQ way.
}

static void AudioQueueFeedInput(void *custom_data, AudioQueueRef queue, AudioQueueBufferRef buffer, const AudioTimeStamp *start_time, UInt32 number_packets_descriptions, const AudioStreamPacketDescription *packets_descriptions ) {
    // We don't feed buffer the normal AQ way.
}

AudioQueueUnit*
AudioQueueCreateInput(int num_channels, int sample_rate, int bits_per_sample, int buffer_size) {
    AudioQueueUnit *unit = new AudioQueueUnit();
    memset(unit, 0, sizeof(*unit));

    unit->format.mSampleRate       = sample_rate;
    unit->format.mFormatID         = kAudioFormatLinearPCM;
    unit->format.mFormatFlags      = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    unit->format.mBitsPerChannel   = 8 * sizeof(float);
    unit->format.mChannelsPerFrame = num_channels;
    unit->format.mBytesPerFrame    = sizeof(float) * num_channels;
    unit->format.mFramesPerPacket  = 1;
    unit->format.mBytesPerPacket   = unit->format.mBytesPerFrame * unit->format.mFramesPerPacket;
    unit->format.mReserved         = 0;
    AudioQueueNewInput(&unit->format, AudioQueueFeedInput, NULL, NULL, NULL, 0, &unit->queue);
    
    for (int i = 0; i < AUDIOQUEUE_BUFFERS_COUNT; ++i) {
        AudioQueueAllocateBuffer(unit->queue, buffer_size, unit->buffers + i);
        AudioQueueEnqueueBuffer(unit->queue, unit->buffers[i], 0, NULL);
    }
    
    return unit;
}
AudioQueueUnit*
AudioQueueCreateOutput(int num_channels, int sample_rate, int bits_per_sample, int buffer_size) {
    AudioQueueUnit *unit = new AudioQueueUnit();
    memset(unit, 0, sizeof(*unit));

    unit->format.mSampleRate       = sample_rate;
    unit->format.mFormatID         = kAudioFormatLinearPCM;
    unit->format.mFormatFlags      = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    unit->format.mBitsPerChannel   = 8 * sizeof(float);
    unit->format.mChannelsPerFrame = num_channels;
    unit->format.mBytesPerFrame    = sizeof(float) * num_channels;
    unit->format.mFramesPerPacket  = 1;
    unit->format.mBytesPerPacket   = unit->format.mBytesPerFrame * unit->format.mFramesPerPacket;
    unit->format.mReserved         = 0;
    AudioQueueNewOutput(&unit->format, AudioQueueFeedOutput, NULL, NULL, NULL, 0, &unit->queue);
    
    return unit;
}

void
AudioQueueRecord(AudioQueueUnit *unit) {
    if (unit->is_running) {
        AudioQueueStart(unit->queue, NULL);
        unit->is_running = true;
    }
}

void
AudioQueuePlay(AudioQueueUnit *unit, int audio_bytes, unsigned char* data) {
    if (unit->queue == NULL || data == NULL) {
        return;
    }
    
    int next_buffer = unit->buffer_count++ % AUDIOQUEUE_BUFFERS_COUNT;
    AudioQueueBufferRef *buffer = unit->buffers + next_buffer;
    
    if ((*buffer) == NULL) {
        AudioQueueAllocateBuffer(unit->queue, audio_bytes, buffer);
        (*buffer)->mAudioDataByteSize = audio_bytes;
    }
    
    // Force enqueue here
    memcpy((*buffer)->mAudioData, data, audio_bytes);
    AudioQueueEnqueueBuffer(unit->queue, *buffer, 0, NULL);
    
    if (!unit->is_running) {
        AudioQueueStart(unit->queue, NULL);
        unit->is_running = true;
    }
}

void
AudioQueueStop(AudioQueueUnit *unit) {
    if (unit->is_running) {
        AudioQueueStop(unit->queue, true);
        unit->is_running = false;
    }
}

void
AudioQueueFree(AudioQueueUnit *unit) {
    AudioQueueDispose(unit->queue, true);
    delete unit;
}

int
AudioQueueGetPlayedSamples(AudioQueueUnit *unit) {
    AudioTimeStamp time_stamp;
    AudioQueueGetCurrentTime(unit->queue, NULL, &time_stamp, NULL);
    return time_stamp.mSampleTime;
}

bool
AudioQueueIsPlaying(AudioQueueUnit *unit) {
    return unit->is_running;
}


#endif

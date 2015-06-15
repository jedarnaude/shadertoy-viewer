
#ifndef shadertoy_test_AudioQueueHelper_h
#define shadertoy_test_AudioQueueHelper_h

#include <AudioToolbox/AudioQueue.h>

#define AUDIOQUEUE_BUFFERS_COUNT 3

typedef struct {
    AudioQueueRef queue;
    AudioQueueBufferRef buffers[AUDIOQUEUE_BUFFERS_COUNT];
    int buffer_count;
    bool is_running;
} AudioQueueUnit;


static void AudioQueueFeed(void *custom_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    // We don't feed buffer the normal AQ way.
}

AudioQueueUnit*
AudioQueueCreate(int num_channels, int sample_rate, int bits_per_sample, int buffer_size) {
    AudioStreamBasicDescription format;
    format.mSampleRate       = sample_rate;
    format.mFormatID         = kAudioFormatLinearPCM;
    format.mFormatFlags      = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel   = 8 * sizeof(float);
    format.mChannelsPerFrame = num_channels;
    format.mBytesPerFrame    = sizeof(float) * num_channels;
    format.mFramesPerPacket  = 1;
    format.mBytesPerPacket   = format.mBytesPerFrame * format.mFramesPerPacket;
    format.mReserved         = 0;
    
    AudioQueueUnit *unit = new AudioQueueUnit();
    memset(unit, 0, sizeof(*unit));
    AudioQueueNewOutput(&format, AudioQueueFeed, NULL, NULL, NULL, 0, &unit->queue);
    
    return unit;
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

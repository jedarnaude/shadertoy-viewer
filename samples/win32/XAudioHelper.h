#pragma once

#include <xaudio2.h>

IXAudio2* g_engine;
IXAudio2MasteringVoice* g_master;

struct XAudioUnit {
    IXAudio2SourceVoice *source;
    bool is_playing;
};

void
XAudioShutdown(bool release_engine) {
    if (release_engine) {
        g_engine->Release();
    }

    CoUninitialize();
}

void 
XAudioInit() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // create the engine
    if (FAILED(XAudio2Create(&g_engine, XAUDIO2_DEBUG_ENGINE, XAUDIO2_DEFAULT_PROCESSOR))) {
        XAudioShutdown(false);
        abort();
    }

    XAUDIO2_DEBUG_CONFIGURATION debug_audio = {};
    debug_audio.TraceMask = XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_INFO;
    debug_audio.LogThreadID = TRUE;
    debug_audio.LogFileline = TRUE;
    debug_audio.LogTiming = TRUE;
    g_engine->SetDebugConfiguration(&debug_audio);

    //create the mastering voice
    if (FAILED(g_engine->CreateMasteringVoice(&g_master))) {
        XAudioShutdown(true);
        abort();
    }
}


XAudioUnit
XAudioCreate(WORD num_channels, DWORD sample_rate, WORD bits_per_sample) {
    int bytes_per_sample = bits_per_sample / 8;

    WAVEFORMATEX wave_format = {};
    wave_format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wave_format.nChannels = num_channels;
    wave_format.nSamplesPerSec = sample_rate;
    wave_format.nAvgBytesPerSec = sample_rate * num_channels * bytes_per_sample;
    wave_format.nBlockAlign = (num_channels * bits_per_sample) / 8;
    wave_format.wBitsPerSample = bits_per_sample;

    XAudioUnit output = {};

    if (FAILED(g_engine->CreateSourceVoice(&output.source, &wave_format))) {
        XAudioShutdown(true);
        abort();
    }

    return output;
}

void
XAudioPlay(XAudioUnit *info, UINT32 audio_bytes, BYTE* data) {
    if (info->source == NULL || data == NULL) {
        return;
    }

    XAUDIO2_BUFFER xaudio_buffer = {};
    xaudio_buffer.AudioBytes = audio_bytes;
    xaudio_buffer.pAudioData = data;

    if (!info->is_playing) {
        info->source->Start();
        info->is_playing = true;
    }

    info->source->SetVolume(0.1f);
    info->source->SubmitSourceBuffer(&xaudio_buffer);
}

void
XAudioStop(XAudioUnit *info) {
    if (info->source == NULL) {
        return;
    }

    info->source->Stop();
}

UINT64
XAudioGetPlayedSamples(XAudioUnit *info) {
    if (info->source == NULL) {
        return 0;
    }
    
    XAUDIO2_VOICE_STATE voice_state = {};
    info->source->GetState(&voice_state);
	return voice_state.SamplesPlayed;
}

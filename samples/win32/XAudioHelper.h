#pragma once

#include <dsound.h>
#include <mmreg.h>

LPDIRECTSOUND8 direct_sound = NULL;
LPDIRECTSOUNDBUFFER primary_buffer = NULL;

struct DSoundUnit {
    LPDIRECTSOUNDBUFFER buffer;
    WAVEFORMATEX wave_format;
    bool is_playing;
};

void
DSoundShutdown() {
    if (primary_buffer) {
        primary_buffer->Release();
        primary_buffer = NULL;
    }

    if (direct_sound) {
        direct_sound->Release();
        direct_sound = NULL;
    }
}

void 
DSoundInit(HWND hWnd) {
    if (FAILED(DirectSoundCreate8(NULL, &direct_sound, NULL))) {
        abort();
    }

    if (FAILED(direct_sound->SetCooperativeLevel(hWnd, DSSCL_PRIORITY))) {
        abort();
    }

    DSBUFFERDESC buffer_desc;
    buffer_desc.dwSize = sizeof(buffer_desc);
    buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    buffer_desc.dwBufferBytes = 0;
    buffer_desc.dwReserved = 0;
    buffer_desc.lpwfxFormat = NULL;
    buffer_desc.guid3DAlgorithm = GUID_NULL;

    if (FAILED(direct_sound->CreateSoundBuffer(&buffer_desc, &primary_buffer, NULL))) {
        abort();
    }

    // Default setup for primary buffer.
    // TODO(jose): This might backfire if we have audio at different frequency or format, but for test purposes its enough.
    WAVEFORMATEX wave_format;
    wave_format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wave_format.nSamplesPerSec = 44100;
    wave_format.wBitsPerSample = 32;
    wave_format.nChannels = 2;
    wave_format.nBlockAlign = (wave_format.wBitsPerSample / 8) * wave_format.nChannels;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
    wave_format.cbSize = 0;

    if (FAILED(primary_buffer->SetFormat(&wave_format))) {
        abort();
    }
}

DSoundUnit
DSoundCreate(WORD num_channels, DWORD sample_rate, WORD bits_per_sample, DWORD buffer_size) {
    int bytes_per_sample = bits_per_sample / 8;

    DSoundUnit output = {};
    output.wave_format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    output.wave_format.nChannels = num_channels;
    output.wave_format.nSamplesPerSec = sample_rate;
    output.wave_format.nAvgBytesPerSec = sample_rate * num_channels * bytes_per_sample;
    output.wave_format.nBlockAlign = (num_channels * bits_per_sample) / 8;
    output.wave_format.wBitsPerSample = bits_per_sample;

    DSBUFFERDESC buffer_desc = {};
    buffer_desc.dwSize = sizeof(buffer_desc);
    buffer_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    buffer_desc.dwBufferBytes = buffer_size;
    buffer_desc.lpwfxFormat = &output.wave_format;

    if (FAILED(direct_sound->CreateSoundBuffer(&buffer_desc, &output.buffer, 0))) {
        abort();
    }

    return output;
}

bool 
DSoundIsPlaying(DSoundUnit *unit) {
    return unit->is_playing;
}

void
DSoundPlay(DSoundUnit *unit, UINT32 audio_bytes, BYTE* data) {
    if (unit->buffer == NULL || data == NULL) {
        return;
    }

    DWORD play_position;
    DWORD write_position;
    if (unit->buffer->GetCurrentPosition(&play_position, &write_position) == DS_OK) {
        DWORD  audio_ptr1_bytes, audio_ptr2_bytes;
        LPVOID audio_ptr1, audio_ptr2;
        unit->buffer->Lock(0, audio_bytes, &audio_ptr1, &audio_ptr1_bytes, &audio_ptr2, &audio_ptr2_bytes, DSBLOCK_FROMWRITECURSOR);
        
        // TODO(jose): write position is advanced but only by a few samples as its point to a correct positino for us to write at, yet we already wrote enough info for several seconds.
        // we should keep track of this in our DSoundUnit and report it back whenever we are handled a second buffer to work with.
        int bound_ptr1 = min(audio_ptr1_bytes, audio_bytes);
        memcpy(audio_ptr1, data, bound_ptr1);
        if (audio_ptr2 != NULL) {
            int bound_ptr2 = audio_bytes - bound_ptr1;
            memcpy(audio_ptr2, data + bound_ptr1, bound_ptr2);
        }        
        unit->buffer->Unlock(audio_ptr1, audio_ptr1_bytes, audio_ptr2, audio_ptr2_bytes);
    }

    if (!unit->is_playing) {
        // TODO(jose): Audio does loop but sound stops after play time. Shall we handle this inside the API?
        unit->buffer->Play(0, 0, DSBPLAY_LOOPING);
        unit->is_playing = true;
    }
}

void
DSoundStop(DSoundUnit *unit) {
    if (unit->buffer == NULL) {
        return;
    }

    unit->buffer->Stop();
}

UINT64
DSoundGetPlayedSamples(DSoundUnit *unit) {
    if (unit->buffer == NULL) {
        return 0;
    }
    
    DWORD current_play_cursor;
    unit->buffer->GetCurrentPosition(&current_play_cursor, NULL);
	return current_play_cursor / unit->wave_format.nBlockAlign;
}

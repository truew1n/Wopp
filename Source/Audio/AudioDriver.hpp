#ifndef WOPP_AUDIO_DRIVER_HPP
#define WOPP_AUDIO_DRIVER_HPP

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
// Add Linux Equivalent
#endif

#include "Wave.h"


enum class EDriverState : uint8_t {
    NONE,
    OPEN,
    DONE,
    CLOSE
};

typedef struct CallbackParam {
    void (*Function)(void *, EDriverState);
    void *Param;
} CallbackParam;

typedef struct Sample {
    uint8_t *SampleData;
    uint8_t DataSize;
    int16_t BitsPerSample;
    int16_t ChannelCount;
} Sample;

class AudioDriver {
private:
#ifdef _WIN32
    HWAVEOUT HWaveOut;
    WAVEFORMATEX WaveFormatX;
    WAVEHDR WaveHeader;
    wave_t *CurrentSong;

    static void CALLBACK AudioDriverCallback(HWAVEOUT HWaveOut, UINT UMsg, DWORD_PTR DwInstance, DWORD_PTR DwParam0, DWORD_PTR DwParam1);
#elif __linux__
    // Add Linux Equivalent
#endif
public:
    AudioDriver();

    void Assign(wave_t *Song);

    void Open(void (*Routine)(void *, EDriverState), void *Parameter);
    void Prepare();
    void Write();
    void Close();

    void Pause();
    void Play();
};

#endif
#pragma once

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
// Add Linux Equivalent
#endif

#include "MutexVariable.hpp"
#include "Wave.h"

enum class ESeekDirection : uint8_t {
    FORWARD,
    BACKWARD
};

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

class AudioDriver {
private:
#ifdef _WIN32
    HWAVEOUT HWaveOut;
    WAVEFORMATEX WaveFormatX;
    WAVEHDR WaveHeader;
    wave_t *CurrentSong;

    uint64_t StartTime;
    uint64_t CurrentTime;

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

    uint64_t GetTime();
    void SetTime(uint64_t NewTime);
    void Seek(uint64_t Offset, ESeekDirection Direction);
};

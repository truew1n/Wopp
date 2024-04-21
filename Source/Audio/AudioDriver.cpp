#include "AudioDriver.hpp"

#ifdef _WIN32
void CALLBACK AudioDriver::AudioDriverCallback(HWAVEOUT HWaveOut, UINT UMsg, DWORD_PTR DwInstance, DWORD_PTR DwParam0, DWORD_PTR DwParam1)
{
    CallbackParam *CallbackParameter = (CallbackParam *) DwInstance;
    if(!CallbackParameter->Function) return;

    EDriverState DriverState = EDriverState::NONE;
    switch(UMsg) {
        case WOM_OPEN: {
            DriverState = EDriverState::OPEN;
            break;
        }
        case WOM_DONE: {
            DriverState = EDriverState::DONE;
            break;
        }
        case WOM_CLOSE: {
            DriverState = EDriverState::CLOSE;
            break;
        }
    }

    CallbackParameter->Function(CallbackParameter->Param, DriverState);
} 
#elif __linux__
    // Add Linux Equivalent
#endif

AudioDriver::AudioDriver()
{

}

void AudioDriver::Assign(wave_t *Song)
{
    CurrentSong = Song;
}

void AudioDriver::Open(void (*Routine)(void *, EDriverState), void *Parameter)
{
    CallbackParam *FCallbackParameter = (CallbackParam *) malloc(sizeof(CallbackParam));
    if(!FCallbackParameter) {
        std::cerr << "AUDIO_DRIVER: CallbackParam malloc failed!\n";
        return;
    }

    FCallbackParameter->Function = Routine;
    FCallbackParameter->Param = Parameter;

    if(!CurrentSong->is_loaded) {
        std::cerr << "AUDIO_DRIVER: Audio is not loaded!\n";
        return;
    }

#ifdef _WIN32
    WaveFormatX.nSamplesPerSec = CurrentSong->fmt_chunk.sample_rate;
    WaveFormatX.wBitsPerSample = CurrentSong->fmt_chunk.bits_per_sample;
    WaveFormatX.nChannels = CurrentSong->fmt_chunk.num_channels;
    WaveFormatX.wFormatTag = CurrentSong->fmt_chunk.audio_format;
    WaveFormatX.nBlockAlign = CurrentSong->fmt_chunk.block_align;
    WaveFormatX.nAvgBytesPerSec = CurrentSong->fmt_chunk.byte_rate;
    WaveFormatX.cbSize = 0;

    MMRESULT ReuseResult = MMSYSERR_NOERROR;
    ReuseResult = waveOutOpen(
        &HWaveOut, WAVE_MAPPER, &WaveFormatX,
        (DWORD_PTR) &AudioDriver::AudioDriverCallback,
        (DWORD_PTR) FCallbackParameter,
        CALLBACK_FUNCTION
    ); if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_DRIVER: Audio device could not be opened!\n";
        return;
    }
#elif
    // Add Linux Equivalent
#endif
}

void AudioDriver::Prepare()
{
#ifdef _WIN32
    WaveHeader.lpData = CurrentSong->data_chunk.data;
    WaveHeader.dwBufferLength = CurrentSong->data_chunk.subchunk_size;
    WaveHeader.dwBytesRecorded = 0;
    WaveHeader.dwUser = 0;
    WaveHeader.dwFlags = 0;
    WaveHeader.dwLoops = 0;

    MMRESULT ReuseResult = waveOutPrepareHeader(
        HWaveOut,
        &WaveHeader,
        sizeof(WAVEHDR)
    ); if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_DRIVER: Preparation failed!\n";
        waveOutClose(HWaveOut);
        return;
    }
#elif __linux__
    // Add Linux Equivalent
#endif
}

void AudioDriver::Write()
{
#ifdef _WIN32    
    MMRESULT ReuseResult = waveOutWrite(
        HWaveOut,
        &WaveHeader,
        sizeof(WAVEHDR)
    ); if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_DRIVER: Writing to audio device failed!\n";
        waveOutUnprepareHeader(HWaveOut, &WaveHeader, sizeof(WAVEHDR));
        waveOutClose(HWaveOut);
        return;
    }
#elif __linux__
    // Add Linux Equivalent
#endif
}

void AudioDriver::Close()
{
#ifdef _WIN32
    waveOutClose(HWaveOut);
#elif __linux__

#endif
}

void AudioDriver::Pause()
{
#ifdef _WIN32
    MMRESULT ReuseResult = waveOutPause(HWaveOut);
    if (ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: Pausing WaveOut device failed!\n";
        return;
    }
#elif __linux__
#endif
}

void AudioDriver::Play()
{
#ifdef _WIN32
    MMRESULT ReuseResult = waveOutRestart(HWaveOut);
    if (ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: Pausing WaveOut device failed!\n";
        return;
    }
#elif __linux__
#endif
}

uint64_t GetTime()
{

}

void SetTime(uint64_t NewTime)
{

}

void Seek(uint64_t Offset, ESeekDirection Direction)
{

}
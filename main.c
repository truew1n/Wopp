#include <stdio.h>
#include <Windows.h>


#include "wave.h"

int main(void)
{
    wave_t wave_file = open_wave_file("audio/audio.wav");
    
    WAVEFORMATEX wfx = {0};
    wfx.nSamplesPerSec = wave_file.fmt_chunk.sample_rate;
    wfx.wBitsPerSample = wave_file.fmt_chunk.bits_per_sample;
    wfx.nChannels = wave_file.fmt_chunk.num_channels;
    wfx.wFormatTag = wave_file.fmt_chunk.audio_format;
    wfx.nBlockAlign = wave_file.fmt_chunk.block_align;
    wfx.nAvgBytesPerSec = wave_file.fmt_chunk.byte_rate;
    wfx.cbSize = 0;
    

    HWAVEOUT hWaveOut = NULL;
    MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: Cannot open waveOut device!\n");
        free(wave_file.data_chunk.data);
        exit(-1);
    }

    WAVEHDR header = {0};
    header.lpData = wave_file.data_chunk.data;
    header.dwBufferLength = wave_file.data_chunk.subchunk_size;
    header.dwBytesRecorded = 0;
    header.dwUser = 0;
    header.dwFlags = 0;
    header.dwLoops = 0;
 
    result = waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: In preparing waveOut header!\n");
        waveOutClose(hWaveOut);
        free(wave_file.data_chunk.data);
        exit(-1);
    }
 
    result = waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: While writing to waveOut device!\n");
        waveOutClose(hWaveOut);
        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
        free(wave_file.data_chunk.data);
        exit(-1);
    }
    
    while (header.dwFlags) {
        Sleep(10);
    }
 
    waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    waveOutClose(hWaveOut);
    free(wave_file.data_chunk.data);
    return 0;
}
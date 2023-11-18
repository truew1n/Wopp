#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>

#define RIFF_CHUNK_ID 0x52494646
#define WAVE_CHUNK_ID 0x57415645
#define FMT_CHUNK_ID 0x666d7420
#define DATA_CHUNK_ID 0x64617461

#define PCM_AUDIO_FORMAT 1

typedef struct riff_chunk_t {
    int32_t chunk_id;
    int32_t chunk_size;
    int32_t format;
} riff_chunk_t;

typedef struct fmt_chunk_t {
    int32_t subchunk_id;
    int32_t subchunk_size;
    int16_t audio_format;
    int16_t num_channels;
    int32_t sample_rate;
    int32_t byte_rate;
    int16_t block_align;
    int16_t bits_per_sample;
} fmt_chunk_t;

typedef struct data_chunk_t {
    int32_t subchunk_id;
    int32_t subchunk_size;
    int8_t *data;
} data_chunk_t;

typedef struct wave_t {
    riff_chunk_t riff_chunk;
    fmt_chunk_t fmt_chunk;
    data_chunk_t data_chunk;
} wave_t;

/*
    Little endian int32_t to Big endian int32_t
*/
int32_t le_to_be(int32_t le_int)
{
    int32_t be_int = 0;
    int8_t size = sizeof(le_int);
    for(int8_t i = 0; i < size; ++i) {
        ((int8_t *) &be_int)[i] = ((int8_t *) &le_int)[size - 1 - i];
    }
    return be_int;
}

int32_t get_file_size(FILE *file)
{
    fseek(file, 0L, SEEK_END);
    int32_t size = ftell(file);
    rewind(file);
    return size;
}

void print_wave_info(wave_t *wave_file)
{
    printf("Audio Format: %i\n", wave_file->fmt_chunk.audio_format);
    printf("Num Channels: %i\n", wave_file->fmt_chunk.num_channels);
    printf("Sample Rate: %i\n", wave_file->fmt_chunk.sample_rate);
    printf("Byte Rate: %i\n", wave_file->fmt_chunk.byte_rate);
    printf("Block Align: %i\n", wave_file->fmt_chunk.block_align);
    printf("Bits Per Sample: %i\n", wave_file->fmt_chunk.bits_per_sample);
}

wave_t open_wave_file(const char *filepath)
{
    FILE *file = fopen(filepath, "rb");

    if(!file) {
        fprintf(stderr, "FILE_STREAM: Cannot open a file!\nFile: %s\n", filepath);
        exit(-1);
    }

    int32_t calculated_chunk_size = get_file_size(file) - 8;

    wave_t wave_file = {0};
    riff_chunk_t riff_chunk = {0};
    fmt_chunk_t fmt_chunk = {0};
    data_chunk_t data_chunk = {0};

    fread(&riff_chunk, sizeof(riff_chunk), 1, file);

    if(le_to_be(riff_chunk.chunk_id) != RIFF_CHUNK_ID) {
        fprintf(stderr, "RIFF_CHUNK_ID:\nGot: 0x%04x\nExpected: 0x%04x\n", riff_chunk.chunk_id, RIFF_CHUNK_ID);
        exit(-1);
    }

    if(riff_chunk.chunk_size != calculated_chunk_size) {
        fprintf(stderr, "RIFF_CHUNK_SIZE: Chunk size is not equal to calculated chunk size\nGot: 0x%04x\nExpected: 0x%04x\n", riff_chunk.chunk_size, calculated_chunk_size);
        exit(-1);
    }

    if(le_to_be(riff_chunk.format) != WAVE_CHUNK_ID) {
        fprintf(stderr, "WAVE_CHUNK_ID:\nGot: 0x%04x\nExpected: 0x%04x\n", riff_chunk.format, WAVE_CHUNK_ID);
        exit(-1);
    }

    fread(&fmt_chunk, sizeof(fmt_chunk), 1, file);

    if(le_to_be(fmt_chunk.subchunk_id) != FMT_CHUNK_ID) {
        fprintf(stderr, "FMT_SUBCHUNK_ID:\nGot: 0x%04x\nExpected: 0x%04x\n", fmt_chunk.subchunk_id, FMT_CHUNK_ID);
        exit(-1);
    }

    if(fmt_chunk.audio_format != PCM_AUDIO_FORMAT)
    {
        fprintf(stderr, "AUDIO_FORMAT:\nUnsupported audio format\nSupported audio formats: PCM\n");
        exit(-1);
    }

    int32_t calculated_byte_rate = fmt_chunk.sample_rate * fmt_chunk.num_channels * fmt_chunk.bits_per_sample / 8;
    if(fmt_chunk.byte_rate != calculated_byte_rate) {
        fprintf(stderr, "BYTE_RATE:\nByte rate is not equal to calculated byte rate\nGot: %i\nExpected: %i\n", fmt_chunk.byte_rate, calculated_byte_rate);
        exit(-1);
    }

    int32_t calculated_block_align = fmt_chunk.num_channels * fmt_chunk.bits_per_sample / 8;
    if(fmt_chunk.block_align != calculated_block_align) {
        fprintf(stderr, "BLOCK_ALIGN:\nBlock align is not equal to calculated block align\nGot: %i\nExpected: %i\n", fmt_chunk.block_align, calculated_block_align);
        exit(-1);
    }

    fread(&data_chunk.subchunk_id, sizeof(data_chunk.subchunk_id), 1, file);

    if(le_to_be(data_chunk.subchunk_id) != DATA_CHUNK_ID) {
        fprintf(stderr, "DATA_SUBCHUNK_ID:\nGot: 0x%04x\nExpected: 0x%04x\n", data_chunk.subchunk_id, DATA_CHUNK_ID);
        exit(-1);
    }

    fread(&data_chunk.subchunk_size, sizeof(data_chunk.subchunk_size), 1, file);

    int32_t calculated_data_subchunk_size = calculated_chunk_size - sizeof(fmt_chunk) - 12;
    if(data_chunk.subchunk_size != calculated_data_subchunk_size) {
        fprintf(stderr, "DATA_SUBCHUNK_ID:\nGot: 0x%04x\nExpected: 0x%04x\n", data_chunk.subchunk_size, calculated_data_subchunk_size);
        exit(-1);
    }

    data_chunk.data = (int8_t *) malloc(sizeof(data_chunk.data) * data_chunk.subchunk_size);
    fread(data_chunk.data, data_chunk.subchunk_size, 1, file);
    
    fclose(file);

    wave_file.riff_chunk = riff_chunk;
    wave_file.fmt_chunk = fmt_chunk;
    wave_file.data_chunk = data_chunk;

    return wave_file;
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WOM_DONE) {
        *((int32_t *) dwInstance) = 0;
    }
}

void play(wave_t *wave_file)
{
    int32_t audio_loop = 1;
    int32_t paused = 0;
    WAVEFORMATEX wfx = {0};
    wfx.nSamplesPerSec = wave_file->fmt_chunk.sample_rate;
    wfx.wBitsPerSample = wave_file->fmt_chunk.bits_per_sample;
    wfx.nChannels = wave_file->fmt_chunk.num_channels;
    wfx.wFormatTag = wave_file->fmt_chunk.audio_format;
    wfx.nBlockAlign = wave_file->fmt_chunk.block_align;
    wfx.nAvgBytesPerSec = wave_file->fmt_chunk.byte_rate;
    wfx.cbSize = 0;

    HWAVEOUT hWaveOut = NULL;
    MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR) waveOutProc, (DWORD_PTR) &audio_loop, CALLBACK_FUNCTION);
    if(result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: Cannot open waveOut device!\n");
        free(wave_file->data_chunk.data);
        exit(-1);
    }

    WAVEHDR header = {0};
    header.lpData = wave_file->data_chunk.data;
    header.dwBufferLength = wave_file->data_chunk.subchunk_size;
    header.dwBytesRecorded = 0;
    header.dwUser = 0;
    header.dwFlags = 0;
    header.dwLoops = 0;

    result = waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    if(result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: In preparing waveOut header!\n");
        waveOutClose(hWaveOut);
        free(wave_file->data_chunk.data);
        exit(-1);
    }
 
    result = waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    if(result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: While writing to waveOut device!\n");
        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
        free(wave_file->data_chunk.data);
        exit(-1);
    }
    
    while(audio_loop);
 
    waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    waveOutClose(hWaveOut);
}
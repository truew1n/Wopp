#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>

#define RIFF_CHUNK_ID 0x52494646
#define WAVE_CHUNK_ID 0x57415645
#define FMT_CHUNK_ID 0x666d7420
#define DATA_CHUNK_ID 0x64617461

#define LIST_CHUNK_ID 0x4C495354

#define PCM_AUDIO_FORMAT 1

typedef char cint8_t;

typedef struct wave_riff_chunk_t {
    int32_t chunk_id;
    int32_t chunk_size;
    int32_t format;
} wave_riff_chunk_t;

typedef struct wave_fmt_chunk_t {
    int32_t subchunk_id;
    int32_t subchunk_size;
    int16_t audio_format;
    int16_t num_channels;
    int32_t sample_rate;
    int32_t byte_rate;
    int16_t block_align;
    int16_t bits_per_sample;
} wave_fmt_chunk_t;

typedef struct wave_data_chunk_t {
    int32_t subchunk_id;
    int32_t subchunk_size;
    cint8_t *data;
} wave_data_chunk_t;

typedef struct wave_t {
    wave_riff_chunk_t riff_chunk;
    wave_fmt_chunk_t fmt_chunk;
    wave_data_chunk_t data_chunk;
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

void wave_print(wave_t *wave_file)
{
    printf("Audio Format: %i\n", wave_file->fmt_chunk.audio_format);
    printf("Num Channels: %i\n", wave_file->fmt_chunk.num_channels);
    printf("Sample Rate: %i\n", wave_file->fmt_chunk.sample_rate);
    printf("Byte Rate: %i\n", wave_file->fmt_chunk.byte_rate);
    printf("Block Align: %i\n", wave_file->fmt_chunk.block_align);
    printf("Bits Per Sample: %i\n", wave_file->fmt_chunk.bits_per_sample);
}

wave_t wave_open(const char *filepath)
{
    FILE *file = fopen(filepath, "rb");

    if(!file) {
        fprintf(stderr, "FILE_STREAM: Cannot open a file!\nFile: %s\n", filepath);
        exit(-1);
    }

    int32_t calculated_chunk_size = get_file_size(file) - 8;

    wave_t wave_file = {0};
    wave_riff_chunk_t riff_chunk = {0};
    wave_fmt_chunk_t fmt_chunk = {0};
    wave_data_chunk_t data_chunk = {0};

    int32_t chunk_id = 0;

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

    fread(&chunk_id, sizeof(chunk_id), 1, file);
    switch(le_to_be(chunk_id)) {
        case LIST_CHUNK_ID: {
            int32_t list_subchunk_size = 0;
            fread(&list_subchunk_size, sizeof(list_subchunk_size), 1, file);
            calculated_chunk_size -= (list_subchunk_size + 8);
            fseek(file, list_subchunk_size, SEEK_CUR);
            break;
        }
        case DATA_CHUNK_ID: {
            int32_t current_file_pointer_position = ftell(file);
            fseek(file, current_file_pointer_position - sizeof(chunk_id), SEEK_SET);
            break;
        }
    }

    fread(&data_chunk.subchunk_id, sizeof(data_chunk.subchunk_id), 1, file);

    if(le_to_be(data_chunk.subchunk_id) != DATA_CHUNK_ID) {
        fprintf(stderr, "DATA_SUBCHUNK_ID:\nGot: 0x%04x\nExpected: 0x%04x\n", data_chunk.subchunk_id, DATA_CHUNK_ID);
        exit(-1);
    }

    fread(&data_chunk.subchunk_size, sizeof(data_chunk.subchunk_size), 1, file);

    int32_t calculated_data_subchunk_size = calculated_chunk_size - sizeof(fmt_chunk) - 12;
    if(data_chunk.subchunk_size > calculated_data_subchunk_size) {
        fprintf(stderr, "DATA_SUBCHUNK_SIZE:\nGot: 0x%04x\nExpected: 0x%04x\n", data_chunk.subchunk_size, calculated_data_subchunk_size);
        exit(-1);
    }

    data_chunk.data = (cint8_t *) malloc(sizeof(data_chunk.data) * data_chunk.subchunk_size);
    fread(data_chunk.data, data_chunk.subchunk_size, 1, file);
    
    fclose(file);

    wave_file.riff_chunk = riff_chunk;
    wave_file.fmt_chunk = fmt_chunk;
    wave_file.data_chunk = data_chunk;

    return wave_file;
}
#include <stdio.h>


#include "wave.h"


int main(void)
{
    wave_t wave_file = open_wave_file("audio/audio1.wav");
    
    play(&wave_file);

    free(wave_file.data_chunk.data);
    return 0;
}
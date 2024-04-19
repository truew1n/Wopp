#include "AudioController.hpp"


void AudioController::SetupWaveFormatX(WAVEFORMATEX *WaveFormatX, wave_t *CurrentSong)
{
    WaveFormatX->nSamplesPerSec = CurrentSong->fmt_chunk.sample_rate;
    WaveFormatX->wBitsPerSample = CurrentSong->fmt_chunk.bits_per_sample;
    WaveFormatX->nChannels = CurrentSong->fmt_chunk.num_channels;
    WaveFormatX->wFormatTag = CurrentSong->fmt_chunk.audio_format;
    WaveFormatX->nBlockAlign = CurrentSong->fmt_chunk.block_align;
    WaveFormatX->nAvgBytesPerSec = CurrentSong->fmt_chunk.byte_rate;
    WaveFormatX->cbSize = 0;
}

void AudioController::SetupWaveHeader(WAVEHDR *WaveHeader, wave_t *CurrentSong)
{
    WaveHeader->lpData = CurrentSong->data_chunk.data;
    WaveHeader->dwBufferLength = CurrentSong->data_chunk.subchunk_size;
    WaveHeader->dwBytesRecorded = 0;
    WaveHeader->dwUser = 0;
    WaveHeader->dwFlags = 0;
    WaveHeader->dwLoops = 0;
}

void CALLBACK AudioController::AudioStreamCallback(HWAVEOUT HWaveOut, UINT UMsg, DWORD_PTR DwInstance, DWORD_PTR DwParam0, DWORD_PTR DwParam1)
{
    switch(UMsg) {
        case WOM_DONE: {
            AudioController *Controller = (AudioController *) DwInstance;
            Controller->AudioFinishedEvent.Send();
        }
    }
}

DWORD WINAPI AudioController::AudioStream(LPVOID LParam)
{
    AudioController *Controller = (AudioController *) LParam;
    
    if(!Controller->CurrentSong.is_loaded) return 2;

    Controller->AudioStreamMutex.Lock();

    SetupWaveFormatX(&Controller->WaveFormatX, &Controller->CurrentSong);

    Controller->AudioFinishedEvent = Event();
    Controller->HWaveOut = NULL;

    MMRESULT ReuseResult = MMSYSERR_NOERROR;
    ReuseResult = waveOutOpen(
        &Controller->HWaveOut, WAVE_MAPPER, &Controller->WaveFormatX,
        (DWORD_PTR) &AudioController::AudioStreamCallback,
        (DWORD_PTR) Controller,
        CALLBACK_FUNCTION
    ); if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: WaveOut device could not be opened!\n";
        return 2;
    }

    SetupWaveHeader(&Controller->WaveHeader, &Controller->CurrentSong);
    

    ReuseResult = waveOutPrepareHeader(
        Controller->HWaveOut,
        &Controller->WaveHeader,
        sizeof(WAVEHDR)
    ); if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: Preparation of WaveHeader failed!\n";
        waveOutClose(Controller->HWaveOut);
        return 2;
    }


    ReuseResult = waveOutWrite(
        Controller->HWaveOut,
        &Controller->WaveHeader,
        sizeof(WAVEHDR)
    ); if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: Writing to WaveOut device failed!\n";
        waveOutUnprepareHeader(Controller->HWaveOut, &Controller->WaveHeader, sizeof(WAVEHDR));
        waveOutClose(Controller->HWaveOut);
        return 2;
    }

    Controller->StartTime = timeGetTime();
    Controller->CurrentTime = Controller->StartTime;
    Controller->AudioStreamState.Set(EAudioStreamState::PLAYING);
    

    // Waiting for playback to finish
    Controller->AudioFinishedEvent.Wait();
    Controller->AudioFinishedEvent.Free();
    
    waveOutClose(Controller->HWaveOut);
    
    Controller->AudioStreamState.Set(EAudioStreamState::IDLE);
    Controller->BAudioStreamCreated.Set(false);

    Controller->AudioStreamMutex.Unlock();
    
    return 100;
}

DWORD WINAPI AudioController::QueueLoop(LPVOID LParam)
{
    AudioController *Controller = (AudioController *) LParam;
    
    Controller->QueueLoopMutex.Lock();
    if(Controller->BAudioStreamCreated.Get()) return 0;
    if(!(Controller->BLoop.Get() || Controller->BQueueLoop.Get())) {
        Controller->Load();
    }

    Controller->BQueueLoopCreated.Set(true);
    do {
        Controller->AudioStreamMutex.Lock();
        if(!Controller->BAudioStreamCreated.Get()) {
            if(Controller->BQueueLoop.Get()) {
                Controller->Load();
                Controller->Skip(ESongQueueSkipOption::FORWARD);
            }
            if (Controller->HAudioStream) {
                CloseHandle(Controller->HAudioStream);
                Controller->HAudioStream = NULL;
            }
            Controller->HAudioStream = CreateThread(NULL, 0, &AudioController::AudioStream, (LPVOID) Controller, 0, NULL);
            
            Controller->BAudioStreamCreated.Set(true);
        }
        Controller->AudioStreamMutex.Unlock();
    } while((Controller->BLoop.Get() || Controller->BQueueLoop.Get()) && !Controller->BInterrupt.Get());

    Controller->BQueueLoopCreated.Set(false);
    Controller->QueueLoopMutex.Unlock();
    return 0;
}

AudioController::AudioController()
{
    // Audio Details

    SongPathQueue = std::vector<std::wstring>();
    CurrentSongIndex = 0;
    CurrentSongPath = std::wstring();
    CurrentSong = {0};
    
    // WaveOut Stuff

    WaveFormatX = {0};
    HWaveOut = NULL;
    WaveHeader = {0};

    DWORD StartTime = 0LU;
    DWORD CurrentTime = 0LU;

    // Mutex

    AudioStreamMutex = Mutex();
    QueueLoopMutex = Mutex();

    // Threads

    HAudioStream = NULL;
    HQueueLoop = NULL;
    

    // States

    BLoop = MutexVariable<bool>(false);
    BQueueLoop = MutexVariable<bool>(true);
    BQueueLoopCreated = MutexVariable<bool>(false);
    BInterrupt = MutexVariable<bool>(false);

    BAudioStreamCreated = MutexVariable<bool>(false);
    AudioStreamState = MutexVariable<EAudioStreamState>(EAudioStreamState::IDLE);
}

void AudioController::Free()
{
    BInterrupt.Set(true);
    Stop();

    QueueLoopMutex.Lock();
    QueueLoopMutex.Unlock();
    AudioStreamMutex.Free();
    QueueLoopMutex.Free();
    CloseHandle(HQueueLoop);

    BLoop.Free();
    BQueueLoop.Free();
    BQueueLoopCreated.Free();
    BInterrupt.Free();

    BAudioStreamCreated.Free();
    AudioStreamState.Free();

    free(CurrentSong.data_chunk.data);
}

AudioController::~AudioController()
{

}

void AudioController::Load()
{
    if(AudioStreamState.Get() == EAudioStreamState::IDLE) {
        if(CurrentSong.is_loaded) {
            free(CurrentSong.data_chunk.data);
        }
        CurrentSongPath = SongPathQueue.at(CurrentSongIndex);
        CurrentSong = wave_open(CurrentSongPath.c_str());
    }
}

void AudioController::Start()
{
    if(BQueueLoopCreated.Get()) return;
    
    HQueueLoop = CreateThread(NULL, 0, &AudioController::QueueLoop, (LPVOID) this, 0, 0);
    
}

void AudioController::Stop()
{
    switch(AudioStreamState.Get()) {
        case EAudioStreamState::PAUSED: __fallthrough;
        case EAudioStreamState::PLAYING: {
            AudioStreamState.Set(EAudioStreamState::STOPPED);
            AudioFinishedEvent.Send();
            break;
        }
    }
}

void AudioController::Pause()
{
    if(AudioStreamState.Get() == EAudioStreamState::PLAYING) {
        MMRESULT ReuseResult = waveOutPause(HWaveOut);
        if (ReuseResult != MMSYSERR_NOERROR) {
            std::cerr << "AUDIO_STREAM: Pausing WaveOut device failed!\n";
            return;
        }
        AudioStreamState.Set(EAudioStreamState::PAUSED);
    }
}

void AudioController::Play()
{
    if(AudioStreamState.Get() == EAudioStreamState::PAUSED) {
        MMRESULT ReuseResult = waveOutRestart(HWaveOut);
        if (ReuseResult != MMSYSERR_NOERROR) {
            std::cerr << "AUDIO_STREAM: Unpausing WaveOut device failed!\n";
            return;
        }
        AudioStreamState.Set(EAudioStreamState::PLAYING);
    }
}

uint64_t AudioController::GetTime()
{
    uint64_t ReturnTime = 0LLU;
    
    switch(AudioStreamState.Get()) {
        case EAudioStreamState::PLAYING: {
            CurrentTime = timeGetTime();
            __fallthrough;
        }
        case EAudioStreamState::PAUSED: {
            ReturnTime = (CurrentTime - StartTime);
            break;
        }
        default: return 0LLU;
    }
    return ReturnTime;
}

void AudioController::SetTime(uint64_t NewTime)
{
    
}

void AudioController::Seek(uint64_t Offset, ESeekDirection Direction)
{

}

uint64_t AudioController::RotateIndex(uint64_t Index)
{
    return Index % SongPathQueue.size();
}

void AudioController::Add(std::wstring SongPath)
{
    SongPathQueue.push_back(SongPath);
}

void AudioController::Remove(uint64_t Index)
{
    SongPathQueue.erase(SongPathQueue.begin() + RotateIndex(Index));
}

void AudioController::Skip(ESongQueueSkipOption Option)
{
    switch(Option) {
        case ESongQueueSkipOption::BACKWARD: {
            CurrentSongIndex--;
            break;
        }
        case ESongQueueSkipOption::FORWARD: __fallthrough;
        default: {
            CurrentSongIndex++;
            break;
        }
    }
    CurrentSongIndex = RotateIndex(CurrentSongIndex);
    CurrentSongPath = SongPathQueue.at(CurrentSongIndex);
}

void AudioController::Last()
{
    Skip(ESongQueueSkipOption::BACKWARD);
}

void AudioController::Next()
{
    Skip(ESongQueueSkipOption::FORWARD);
}
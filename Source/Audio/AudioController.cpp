#include "AudioController.hpp"


bool AudioController::GetBLoop(AudioController *Controller)
{
    bool ReturnBool = true;
    WaitForSingleObject(Controller->HBLoopMutex, INFINITE);
    ReturnBool = Controller->BLoop;
    ReleaseMutex(Controller->HBLoopMutex);
    return ReturnBool;
}

void AudioController::SetBLoop(AudioController *Controller, bool NewValue)
{
    WaitForSingleObject(Controller->HBLoopMutex, INFINITE);
    Controller->BLoop = NewValue;
    ReleaseMutex(Controller->HBLoopMutex);
}

bool AudioController::GetBQueueLoop(AudioController *Controller)
{
    bool ReturnBool = true;
    WaitForSingleObject(Controller->HBQueueLoopMutex, INFINITE);
    ReturnBool = Controller->BQueueLoop;
    ReleaseMutex(Controller->HBQueueLoopMutex);
    return ReturnBool;
}

void AudioController::SetBQueueLoop(AudioController *Controller, bool NewValue)
{
    WaitForSingleObject(Controller->HBQueueLoopMutex, INFINITE);
    if(GetBLoop(Controller)) Controller->BQueueLoop = false;
    else Controller->BQueueLoop = NewValue;
    ReleaseMutex(Controller->HBQueueLoopMutex);
}

bool AudioController::GetBQueueLoopCreated(AudioController *Controller)
{
    bool ReturnBool = true;
    WaitForSingleObject(Controller->HBCQueueLoopMutex, INFINITE);
    ReturnBool = Controller->BQueueLoopCreated;
    ReleaseMutex(Controller->HBCQueueLoopMutex);
    return ReturnBool;
}

void AudioController::SetBQueueLoopCreated(AudioController *Controller, bool NewValue)
{
    WaitForSingleObject(Controller->HBCAudioStreamMutex, INFINITE);
    Controller->BQueueLoopCreated = NewValue;
    ReleaseMutex(Controller->HBCAudioStreamMutex);
}

bool AudioController::GetBAudioStreamCreated(AudioController *Controller)
{
    bool ReturnBool = true;
    WaitForSingleObject(Controller->HBCAudioStreamMutex, INFINITE);
    ReturnBool = Controller->BAudioStreamCreated;
    ReleaseMutex(Controller->HBCAudioStreamMutex);
    return ReturnBool;
}

void AudioController::SetBAudioStreamCreated(AudioController *Controller, bool NewValue)
{
    WaitForSingleObject(Controller->HBCAudioStreamMutex, INFINITE);
    Controller->BAudioStreamCreated = NewValue;
    ReleaseMutex(Controller->HBCAudioStreamMutex);
}

EAudioStreamState AudioController::GetAudioStreamState(AudioController *Controller)
{
    EAudioStreamState ReturnState = EAudioStreamState::IDLE;
    WaitForSingleObject(Controller->HAudioStreamStateMutex, INFINITE);
    ReturnState = Controller->AudioStreamState;
    ReleaseMutex(Controller->HAudioStreamStateMutex);
    return ReturnState;
}

void AudioController::SetAudioStreamState(AudioController *Controller, EAudioStreamState State)
{
    WaitForSingleObject(Controller->HAudioStreamStateMutex, INFINITE);
    Controller->AudioStreamState = State;
    ReleaseMutex(Controller->HAudioStreamStateMutex);
}

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

DWORD WINAPI AudioController::AudioStream(LPVOID LParam)
{
    AudioController *Controller = (AudioController *) LParam;
    
    if(!Controller->CurrentSong.is_loaded) return 2;

    SetupWaveFormatX(&Controller->WaveFormatX, &Controller->CurrentSong);

    Controller->HWaveOut = NULL;
    MMRESULT ReuseResult = MMSYSERR_NOERROR;
    ReuseResult = waveOutOpen(
        &Controller->HWaveOut, WAVE_MAPPER, &Controller->WaveFormatX,
        NULL,
        (DWORD_PTR) Controller,
        CALLBACK_FUNCTION
    );
    
    if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: WaveOut device could not be opened!\n";
        return 2;
    }

    SetupWaveHeader(&Controller->WaveHeader, &Controller->CurrentSong);
    

    ReuseResult = waveOutPrepareHeader(
        Controller->HWaveOut,
        &Controller->WaveHeader,
        sizeof(WAVEHDR)
    );
    
    if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: Preparation of WaveHeader failed!\n";
        waveOutClose(Controller->HWaveOut);
        return 2;
    }


    ReuseResult = waveOutWrite(
        Controller->HWaveOut,
        &Controller->WaveHeader,
        sizeof(WAVEHDR)
    );
    
    if(ReuseResult != MMSYSERR_NOERROR) {
        std::cerr << "AUDIO_STREAM: Writing to WaveOut device failed!\n";
        waveOutUnprepareHeader(Controller->HWaveOut, &Controller->WaveHeader, sizeof(WAVEHDR));
        waveOutClose(Controller->HWaveOut);
        return 2;
    }

    Controller->StartTime = timeGetTime();
    Controller->CurrentTime = Controller->StartTime;
    SetAudioStreamState(Controller, EAudioStreamState::PLAYING);
    
    while (
        waveOutUnprepareHeader(Controller->HWaveOut, &Controller->WaveHeader, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING ||
        GetAudioStreamState(Controller) == EAudioStreamState::STOPPED
    );
    
    waveOutClose(Controller->HWaveOut);
    SetAudioStreamState(Controller, EAudioStreamState::IDLE);
    SetBAudioStreamCreated(Controller, false);
    return 100;
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

    // Mutexes
    HBLoopMutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );
    HBQueueLoopMutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );
    HBCQueueLoopMutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );

    HAudioStreamStateMutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );
    HBCAudioStreamMutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );

    // Threads
    HAudioStream = NULL;
    HQueueLoop = NULL;
    

    // States
    BLoop = false;
    BQueueLoop = true;
    BQueueLoopCreated = false;

    BAudioStreamCreated = false;
    AudioStreamState = EAudioStreamState::IDLE;
}

AudioController::~AudioController()
{
    CloseHandle(HBLoopMutex);
    CloseHandle(HBQueueLoopMutex);
    CloseHandle(HBCQueueLoopMutex);

    CloseHandle(HAudioStream);
    CloseHandle(HAudioStreamStateMutex);
    CloseHandle(HBCAudioStreamMutex);
}

void AudioController::Load()
{
    if(GetAudioStreamState(this) == EAudioStreamState::IDLE) {
        if(CurrentSong.is_loaded) {
            free(CurrentSong.data_chunk.data);
        }
        CurrentSongPath = SongPathQueue.at(CurrentSongIndex);
        CurrentSong = wave_open(CurrentSongPath.c_str());
    }
}

DWORD WINAPI AudioController::QueueLoop(LPVOID LParam)
{
    AudioController *Controller = (AudioController *) LParam;
    
    if(GetBAudioStreamCreated(Controller)) return 0;

    if(!(GetBLoop(Controller) || GetBQueueLoop(Controller))) {
        Controller->Load();
    }

    SetBQueueLoopCreated(Controller, true);
    
    do {
        if(!GetBAudioStreamCreated(Controller)) {
            if(GetBQueueLoop(Controller)) {
                Controller->Load();
                Controller->Skip(ESongQueueSkipOption::FORWARD);
            }
            if (Controller->HAudioStream) {
                CloseHandle(Controller->HAudioStream);
                Controller->HAudioStream = NULL;
            }
            Controller->HAudioStream = CreateThread(NULL, 0, &AudioController::AudioStream, (LPVOID) Controller, 0, NULL);
            SetBAudioStreamCreated(Controller, true);
        }
    } while(GetBLoop(Controller) || GetBQueueLoop(Controller));
    SetBQueueLoopCreated(Controller, false);
    return 0;
}

void AudioController::Start()
{
    
    if(BQueueLoopCreated) return;
    
    HQueueLoop = CreateThread(NULL, 0, &AudioController::QueueLoop, (LPVOID) this, 0, 0);
    
}

void AudioController::Stop()
{
    switch(GetAudioStreamState(this)) {
        case EAudioStreamState::PAUSED: __fallthrough;
        case EAudioStreamState::PLAYING: {
            SetAudioStreamState(this, EAudioStreamState::STOPPED);
            break;
        }
    }
}

void AudioController::Pause()
{
    if(GetAudioStreamState(this) == EAudioStreamState::PLAYING) {
        MMRESULT ReuseResult = waveOutPause(HWaveOut);
        if (ReuseResult != MMSYSERR_NOERROR) {
            std::cerr << "AUDIO_STREAM: Pausing WaveOut device failed!\n";
            return;
        }
        SetAudioStreamState(this, EAudioStreamState::PAUSED);
    }
}

void AudioController::Play()
{
    if(GetAudioStreamState(this) == EAudioStreamState::PAUSED) {
        MMRESULT ReuseResult = waveOutRestart(HWaveOut);
        if (ReuseResult != MMSYSERR_NOERROR) {
            std::cerr << "AUDIO_STREAM: Unpausing WaveOut device failed!\n";
            return;
        }
        SetAudioStreamState(this, EAudioStreamState::PLAYING);
    }
}

uint64_t AudioController::GetTime()
{
    uint64_t ReturnTime = 0LLU;
    
    switch(GetAudioStreamState(this)) {
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
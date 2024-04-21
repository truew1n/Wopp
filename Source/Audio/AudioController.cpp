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

void AudioController::AudioStreamCallback(void *Parameter, EDriverState DriverState)
{
    switch(DriverState) {
        case EDriverState::DONE: {
            AudioController *Controller = (AudioController *) Parameter;
            Controller->AudioFinishedEvent.Send();
        }
    }
}

uint32_t AudioController::AudioStream(void *Parameter)
{
    AudioController *Controller = (AudioController *) Parameter;
    
    if(!Controller->CurrentSong.is_loaded) return 2;

    Controller->AudioStreamMutex.Lock();

    Controller->AudioFinishedEvent = Event();
    Controller->Driver.Assign(&Controller->CurrentSong);
    Controller->Driver.Open(&AudioController::AudioStreamCallback, (void *) Controller);

    Controller->Driver.Prepare();


    Controller->Driver.Write();

    Controller->StartTime = timeGetTime();
    Controller->CurrentTime = Controller->StartTime;
    Controller->AudioStreamState.Set(EAudioStreamState::PLAYING);
    

    // Waiting for playback to finish
    Controller->AudioFinishedEvent.Wait();
    Controller->AudioFinishedEvent.Free();
    
    Controller->Driver.Close();
    
    Controller->AudioStreamState.Set(EAudioStreamState::IDLE);
    Controller->BAudioStreamCreated.Set(false);

    Controller->AudioStreamMutex.Unlock();
    
    return 100;
}

uint32_t AudioController::QueueLoop(void *Parameter)
{
    AudioController *Controller = (AudioController *) Parameter;
    
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
            Controller->AudioStreamThread.Free();
            Controller->AudioStreamThread = Thread(&AudioController::AudioStream, (void *) Controller);
            
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
    Driver = AudioDriver();

    StartTime = 0LU;
    CurrentTime = 0LU;

    // Mutex

    AudioStreamMutex = Mutex();
    QueueLoopMutex = Mutex();

    // Threads
    AudioStreamThread = Thread();
    QueueLoopThread = Thread();
    

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
    QueueLoopThread.Free();

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
    
    QueueLoopThread = Thread(&AudioController::QueueLoop, (void *) this);
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
        Driver.Pause();
        AudioStreamState.Set(EAudioStreamState::PAUSED);
    }
}

void AudioController::Play()
{
    if(AudioStreamState.Get() == EAudioStreamState::PAUSED) {
        Driver.Play();
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
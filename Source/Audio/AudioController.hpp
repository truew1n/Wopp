#ifndef WOPP_AUDIO_CONTROLLER_HPP
#define WOPP_AUDIO_CONTROLLER_HPP

#include <iostream>
#include <windows.h>
#include <vector>

#include "AudioDriver.hpp"
#include "MutexVariable.hpp"
#include "Thread.hpp"
#include "Event.hpp"
#include "Wave.h"

enum class ESeekDirection : uint8_t {
    FORWARD,
    BACKWARD
};

enum class EAudioStreamState : uint8_t {
    IDLE,
    PLAYING,
    PAUSED,
    STOPPED
};

enum class ESongQueueSkipOption : uint8_t {
    NONE,
    FORWARD,
    BACKWARD
};

class AudioController {
private:
    // Audio Details
    std::vector<std::wstring> SongPathQueue;
    uint64_t CurrentSongIndex;
    std::wstring CurrentSongPath;
    wave_t CurrentSong;

    // Audio Driver
    AudioDriver Driver;

    uint64_t StartTime;
    uint64_t CurrentTime;

    // Mutexes
    Mutex AudioStreamMutex;
    Mutex QueueLoopMutex;
    Event AudioFinishedEvent;
    
    // Threads
    Thread AudioStreamThread;
    Thread QueueLoopThread;

    // States
    MutexVariable<bool> BLoop;
    MutexVariable<bool> BQueueLoop;
    MutexVariable<bool> BQueueLoopCreated;
    MutexVariable<bool> BInterrupt;

    MutexVariable<bool> BAudioStreamCreated;
    MutexVariable<EAudioStreamState> AudioStreamState;

private:
    static void AudioStreamCallback(void *Parameter, EDriverState DriverState);
    
    static uint32_t AudioStream(void *Parameter);
    static uint32_t QueueLoop(void *Parameter);

public:
    AudioController();

    void Free();
    ~AudioController();

    void Load();
    
    void Start();
    void Stop();

    void Pause();
    void Play();

    uint64_t GetTime();
    void SetTime(uint64_t NewTime);
    void Seek(uint64_t Offset, ESeekDirection Direction);

    uint64_t RotateIndex(uint64_t Index);

    void Add(std::wstring SongPath);
    void AddAll(std::wstring *SongPath);
    void Remove(uint64_t Index);

    void Skip(ESongQueueSkipOption Option);
    void Last();
    void Next();
};

#endif
#pragma once

#include <iostream>
#include <windows.h>
#include <vector>

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
public:
    static bool GetBLoop(AudioController *Caller);
    static void SetBLoop(AudioController *Caller, bool NewValue);

    static bool GetBQueueLoop(AudioController *Caller);
    static void SetBQueueLoop(AudioController *Caller, bool NewValue);

    static bool GetBQueueLoopCreated(AudioController *Caller);
    static void SetBQueueLoopCreated(AudioController *Caller, bool NewValue);

    static bool GetBAudioStreamCreated(AudioController *Caller);
    static void SetBAudioStreamCreated(AudioController *Caller, bool NewValue);

    static void SetAudioStreamState(AudioController *Controller, EAudioStreamState State);
    static EAudioStreamState GetAudioStreamState(AudioController *Controller);
private:
    // Audio Details
    std::vector<std::wstring> SongPathQueue;
    uint64_t CurrentSongIndex;
    std::wstring CurrentSongPath;
    wave_t CurrentSong;

    // WaveOut Stuff
    WAVEFORMATEX WaveFormatX;
    HWAVEOUT HWaveOut;
    WAVEHDR WaveHeader;

    DWORD StartTime;
    DWORD CurrentTime;

    // Mutexes & Events
    HANDLE HBLoopMutex;
    HANDLE HBQueueLoopMutex;
    HANDLE HBCQueueLoopMutex; // Mutex for QueueLoopCreated bool state

    HANDLE HAudioStreamMutex;
    HANDLE HAudioStreamStateMutex;
    HANDLE HBCAudioStreamMutex; // Mutex for AudioStreamCreated bool state
    
    HANDLE HAudioFinishedEvent;

    // Threads
    HANDLE HAudioStream;
    HANDLE HQueueLoop;

    // States
    bool BLoop;
    bool BQueueLoop;
    bool BQueueLoopCreated;
    bool BInterrupt;

    bool BAudioStreamCreated;
    EAudioStreamState AudioStreamState;

private:
    static void SetupWaveFormatX(WAVEFORMATEX *WaveFormatX, wave_t *CurrentSong);
    static void SetupWaveHeader(WAVEHDR *WaveHeader, wave_t *CurrentSong);

    static void CALLBACK AudioStreamCallback(HWAVEOUT HWaveOut, UINT UMsg, DWORD_PTR DwInstance, DWORD_PTR DwParam0, DWORD_PTR DwParam1);
    
    static DWORD WINAPI AudioStream(LPVOID LParam);
    static DWORD WINAPI QueueLoop(LPVOID LParam);

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
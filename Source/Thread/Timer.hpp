#ifndef WOPP_TIMER_HPP
#define WOPP_TIMER_HPP

#ifdef _WIN32
#include <windows.h>
#include <stdint.h>
#elif __linux__
#include <eventfd.h>
#endif

enum class TimerType {
    ONESHOT,
    PERIODIC
};

typedef struct TimerParam {
    void (*Function)(void *);
    void *Param;
} TimerParam;

class Timer {
private:
    uint32_t Delay;
    uint32_t Resolution;
    TimerParam *Parameter;
    TimerType Type;

    uint8_t IsSet;
    uint8_t IsActive;
#ifdef _WIN32
    MMRESULT HTimer;

    static void CALLBACK TimerCallback(UINT UID, UINT UMsg, DWORD_PTR DwUser, DWORD_PTR DwParam0, DWORD_PTR DwParam1);
#elif __linux__
    // Add Linux Event Handle
#endif
public:
    Timer();
    Timer(uint32_t Delay, uint32_t Resolution, TimerParam *Parameter, TimerType Type);
    void Set(uint32_t Delay, uint32_t Resolution, TimerParam *Parameter, TimerType Type);
    void Start();
    void Stop();
};

#endif
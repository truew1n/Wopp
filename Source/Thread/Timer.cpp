#include "Timer.hpp"

#ifdef _WIN32
void CALLBACK Timer::TimerCallback(UINT UID, UINT UMsg, DWORD_PTR DwUser, DWORD_PTR DwParam0, DWORD_PTR DwParam1)
{
    TimerParam *Parameter = (TimerParam *) DwUser;
    if(!Parameter->Function) return;

    Parameter->Function(Parameter->Param);
}      
#elif __linux__

#endif

Timer::Timer()
{
    uint32_t Delay = 1000;
    uint32_t Resolution = 0;
    TimerParam *Parameter = nullptr;
    TimerType Type = TimerType::ONESHOT;

    uint8_t IsSet = false;
    IsActive = false;
}

Timer::Timer(uint32_t Delay, uint32_t Resolution, TimerParam *Parameter, TimerType Type)
{
    Set(Delay, Resolution, Parameter, Type);
}

void Timer::Set(uint32_t Delay, uint32_t Resolution, TimerParam *Parameter, TimerType Type)
{
    this->Delay = Delay;
    this->Resolution = Resolution;
    this->Parameter = Parameter;
    this->Type = Type;

    IsSet = true;
    IsActive = false;
}

void Timer::Start()
{
#ifdef _WIN32
    if(IsSet && !IsActive) {
        uint32_t InternalType = TIME_ONESHOT;
        switch (Type) {
            case TimerType::ONESHOT: {
                InternalType = TIME_ONESHOT;
                break;
            }
            case TimerType::PERIODIC: {
                InternalType = TIME_PERIODIC;
                break;
            }
        }
        HTimer = timeSetEvent(Delay, Resolution, &Timer::TimerCallback, (DWORD_PTR) Parameter, InternalType);
        IsActive = true;
    }
#elif __linux__
    // Linux Equivalent
#endif
}

void Timer::Stop()
{
#ifdef _WIN32
    if(IsActive) {
        timeKillEvent(HTimer);
        IsActive = false;
    }
#elif __linux__
    // Linux Equivalent
#endif
}
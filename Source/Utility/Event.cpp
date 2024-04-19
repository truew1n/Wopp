#include "Event.hpp"


Event::Event()
{
#ifdef _WIN32
    HEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
#elif __linux__
    // Add Linux Create Event
#endif
}

void Event::Free()
{
#ifdef _WIN32
    if(HEvent) CloseHandle(HEvent);
#elif __linux__
    // Add Linux Free Handle
#endif
}

void Event::Wait()
{
#ifdef _WIN32
    WaitForSingleObject(HEvent, INFINITE);
#elif __linux__
    // Add Linux Wait Mechanism
#endif
}

void Event::Send()
{
#ifdef _WIN32
    SetEvent(HEvent);
#elif __linux__
    // Add Linux Send Mechanism
#endif
}
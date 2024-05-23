#ifndef WOPP_EVENT_HPP
#define WOPP_EVENT_HPP

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <eventfd.h>
#endif

class Event {
private:
#ifdef _WIN32
    HANDLE HEvent;
#elif __linux__
    // Add Linux Event Handle
#endif
public:
    Event();
    void Free();

    void Wait();
    void Send();
};

#endif
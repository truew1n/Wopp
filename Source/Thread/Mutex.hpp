#ifndef WOPP_MUTEX_HPP
#define WOPP_MUTEX_HPP

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#endif


class Mutex {
private:
#ifdef _WIN32
    HANDLE HMutex;
#elif __linux__
    // Add Linux Mutex Handle
#endif
public:
    Mutex();
    void Free();

    void Lock();
    void Unlock();
};

#endif
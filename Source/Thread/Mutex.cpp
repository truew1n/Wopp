#include "Mutex.hpp"

Mutex::Mutex()
{
#ifdef _WIN32
    HMutex = CreateMutexW(NULL, FALSE, NULL);
#elif __linux__
    // Add Linux Create Mutex
#endif
}

void Mutex::Free()
{
#ifdef _WIN32
    if(HMutex) CloseHandle(HMutex);
#elif __linux__
    // Add Linux Free Handle
#endif
}

void Mutex::Lock()
{
#ifdef _WIN32
    WaitForSingleObject(HMutex, INFINITE);
#elif __linux__
    // Add Linux Locking Mechanism
#endif
}

void Mutex::Unlock()
{
#ifdef _WIN32
    ReleaseMutex(HMutex);
#elif __linux__
    // Add Linux Unlocking Mechanism
#endif
}
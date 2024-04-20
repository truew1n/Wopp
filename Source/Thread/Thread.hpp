#pragma once

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#endif

typedef struct ThreadParam {
    uint32_t (*Function)(void *);
    void *Param;
} ThreadParam;

class Thread {
private:
    ThreadParam *FThreadParameter;
#ifdef _WIN32
    HANDLE HThread;

    static DWORD WINAPI ThreadFunction(LPVOID LParam);
#elif __linux__
    // Add Linux Thread Handle
#endif
public:
    Thread();
    Thread(uint32_t (*Routine)(void *), void *Param);

    void Free();
};
#ifndef WOPP_THREAD_HPP
#define WOPP_THREAD_HPP

#include <iostream>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <pthread.h>
#endif

typedef uint32_t (*ThreadFunc)(void *);
typedef struct ThreadParam {
    ThreadFunc Function;
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
    Thread(ThreadFunc Routine, void *Param);

    void Free();
};

#endif
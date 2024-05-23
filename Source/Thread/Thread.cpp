#include "Thread.hpp"

#ifdef _WIN32
DWORD WINAPI Thread::ThreadFunction(LPVOID LParam)
{
    ThreadParam *ThreadParameter = (ThreadParam *) LParam;
    if(!ThreadParameter->Function) {
        return -1;
    }
    return ThreadParameter->Function(ThreadParameter->Param);
}
#elif __linux__
    // Add Linux ThreadFunction Equivalent
    // See how it's done above, ThreadFunction is private and static
    // I don't know if in linux needs to be like this
#endif

Thread::Thread()
{
    HThread = NULL;
    FThreadParameter = NULL;
}

Thread::Thread(ThreadFunc Routine, void *Param)
{
    FThreadParameter = (ThreadParam *) malloc(sizeof(ThreadParam));
    if(!FThreadParameter) return;

    FThreadParameter->Function = Routine;
    FThreadParameter->Param = Param;
#ifdef _WIN32
    HThread = CreateThread(NULL, 0, &Thread::ThreadFunction, (LPVOID) FThreadParameter, 0, NULL);
#elif __linux__
    // Add Linux CreateThread Equivalent
    // Also if Linux CreateThread need to be started just start it
    // For now we need to start instantly
#endif
}

void Thread::Free()
{
#ifdef _WIN32
    if(HThread) CloseHandle(HThread);
#elif __linux__
    // Add Linux Freeing Thread Resources
#endif
    if(FThreadParameter) free(FThreadParameter);
}
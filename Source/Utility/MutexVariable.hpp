#pragma once

#include <iostream>
#include <windows.h>

template<typename T>
class MutexVariable {
private:
    T Value;
    HANDLE ValueMutex;
public:
    MutexVariable() : Value(T()), ValueMutex(NULL) {}
    
    MutexVariable(T NewValue)
    {
        Value = NewValue;
        ValueMutex = CreateMutexA(NULL, FALSE, NULL);
    }

    void Free()
    {
        if(ValueMutex) CloseHandle(ValueMutex);
    }

    // Destructor caused problems, figure out why
    // ~MutexVariable()
    // {

    // }

    T Get()
    {
        T ReturnValue = T();
        WaitForSingleObject(ValueMutex, INFINITE);
        ReturnValue = Value;
        ReleaseMutex(ValueMutex);
        return ReturnValue;
    }
    
    void Set(T NewValue)
    {
        WaitForSingleObject(ValueMutex, INFINITE);
        Value = NewValue;
        ReleaseMutex(ValueMutex);
    }
};
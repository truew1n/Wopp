#pragma once

#include <iostream>

#include "Mutex.hpp"

template<typename T>
class MutexVariable {
private:
    T Value;
    Mutex ValueMutex;
public:
    MutexVariable() : Value(T()), ValueMutex() {}
    
    MutexVariable(T NewValue)
    {
        Value = NewValue;
        ValueMutex = Mutex();
    }

    void Free()
    {
        ValueMutex.Free();
    }

    // Destructor caused problems, figure out why
    // ~MutexVariable()
    // {

    // }

    T Get()
    {
        T ReturnValue = T();
        ValueMutex.Lock();
        ReturnValue = Value;
        ValueMutex.Unlock();
        return ReturnValue;
    }
    
    void Set(T NewValue)
    {
        ValueMutex.Lock();
        Value = NewValue;
        ValueMutex.Unlock();
    }
};
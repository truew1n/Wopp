#ifndef WOPP_MUTEX_VARIABLE_HPP
#define WOPP_MUTEX_VARIABLE_HPP

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

    T Get()
    {
        T ReturnValue = T();
        ValueMutex.Lock();
        ReturnValue = Value;
        ValueMutex.Unlock();
        return ReturnValue;
    }

    template<typename R>
    R Use(R (*SafeRoutine)(T *))
    {
        R ReturnValue = R();
        ValueMutex.Lock();
        ReturnValue = SafeRoutine(&Value);
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

#endif
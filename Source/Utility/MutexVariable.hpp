#pragma once

#include <iostream>
#include <windows.h>

template<typename T>
class MutexVariable {
private:
    T Value;
    HANDLE ValueMutex;
public:
    MutexVariable(T Value);
    ~MutexVariable();

    T Get();
    void Set(T Value);
};
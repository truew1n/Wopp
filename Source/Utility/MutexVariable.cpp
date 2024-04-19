#include "MutexVariable.hpp"

template<typename T>
MutexVariable<T>::MutexVariable(T Value)
{
    this->Value = Value;
    ValueMutex = CreateMutex(NULL, FALSE, NULL);
}

template<typename T>
MutexVariable<T>::~MutexVariable()
{
    CloseHandle(ValueMutex);
}

template<typename T>
T MutexVariable<T>::Get()
{
    T ReturnValue = (T) {0};
    WaitForSingleObject(ValueMutex, INFINITE);
    ReturnValue = Value;
    ReleaseMutex(ValueMutex);
    return ReturnValue;
}

template<typename T>
void MutexVariable<T>::Set(T Value)
{
    WaitForSingleObject(ValueMutex, INFINITE);
    this->Value = Value;
    ReleaseMutex(ValueMutex);
}
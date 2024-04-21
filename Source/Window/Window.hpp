#pragma once

#include <iostream>
#include "windows.h"

#include "DisplayComponent.hpp"

class Window {
private:
    DisplayComponent Component;
#ifdef _WIN32
    HINSTANCE HInstance;
    WNDCLASSW WindowClass;
    RECT WindowRectangle;

    HWND HWindow;
#elif __linux__

#endif
public:
    Window();
    void Free();

    void Add(DisplayComponent Component);
    void Pack();
};
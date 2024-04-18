#pragma once

#include <iostream>
#include "windows.h"

class Window {
private:
    HINSTANCE HInstance;
    WNDCLASSW WindowClass;
    RECT WindowRectangle;
    int32_t Width;
    int32_t Height;

    HWND HWindow;
public:
    Window();
    ~Window();
};
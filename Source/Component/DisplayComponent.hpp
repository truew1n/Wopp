#pragma once

#include <iostream>

class DisplayComponent {
private:
    int32_t Width;
    int32_t Height;

    void *Display;
public:
    DisplayComponent();
    void Free();

    void *GetDisplay();
};
#include <iostream>
#include <windows.h>

#include "AudioController.hpp"


uint32_t CalcAOB(uint32_t Value, uint32_t AOT)
{
    return (Value / AOT) + ((Value % AOT) > 0);
}

LRESULT CALLBACK WinProcedure(HWND HWnd, UINT UMsg, WPARAM WParam, LPARAM LParam);

int main(void)
{
    AudioController Controller = AudioController();

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind;

    LPCWSTR searchPath = L"Assets\\Audio\\*.wav";
    hFind = FindFirstFileW(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error finding files in directory" << std::endl;
        return 1;
    }

    do {
        std::wcout << findFileData.cFileName << std::endl;
        Controller.Add(L"Assets\\Audio\\" + std::wstring(findFileData.cFileName));
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);

    // Controller.Add(L"Assets\\Audio\\Alle Farben feat. YouNotUs- Please Tell Rosie [Official Video].wav");
    // Controller.Add(L"Assets\\Audio\\Kungs vs Cookin on 3 Burners - This Girl (Official Music Video).wav");
    // Controller.Add(L"Assets\\Audio\\SAIL - AWOLNATION (Unofficial Video).wav");

    Controller.Start();

    ShowWindow(GetConsoleWindow(), SW_HIDE);
    HINSTANCE WinInstance = GetModuleHandleW(NULL);
    
    WNDCLASSW WinClass = {0};
    WinClass.lpszClassName = L"Wopp";
    WinClass.hbrBackground = (HBRUSH) COLOR_WINDOW;
    WinClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WinClass.hInstance = WinInstance;
    WinClass.lpfnWndProc = WinProcedure;

    if(!RegisterClassW(&WinClass)) return -1;

    uint32_t Width = 800;
    uint32_t Height = 600;

    RECT WindowRect = { 0 };
    WindowRect.right = Width;
    WindowRect.bottom = Height;
    WindowRect.left = 0;
    WindowRect.top = 0;

    AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
    HWND Window = CreateWindowW(
        WinClass.lpszClassName,
        L"Wopp - Wave Audio Player",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL, NULL,
        NULL, NULL
    );
    
    GetWindowRect(Window, &WindowRect);

    uint32_t BitmapWidth = Width;
    uint32_t BitmapHeight = Height;

    uint32_t BytesPerPixel = 4;

    uint32_t BitmapTotalSize = BitmapWidth * BitmapHeight;
    uint32_t DisplayTotalSize = BitmapTotalSize * BytesPerPixel;

    void *Display;
    cudaMallocManaged(&Display, DisplayTotalSize);

    uint32_t AOT = 1024;
    uint32_t DisplayAOB = CalcAOB(BitmapTotalSize, AOT);

    BITMAPINFO BitmapInfo;
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(Window);

    MSG msg = { 0 };
    int32_t running = 1;
    while (running) {

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
                case WM_QUIT: {
                    running = 0;
                    break;
                }
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        StretchDIBits(
            hdc, 0, 0,
            BitmapWidth, BitmapHeight,
            0, 0,
            BitmapWidth, BitmapHeight,
            Display, &BitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }

    cudaFree(Display);

    Controller.Free();
    return 0;
}

LRESULT CALLBACK WinProcedure(HWND HWnd, UINT UMsg, WPARAM WParam, LPARAM LParam)
{
    switch (UMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default: {
            return DefWindowProcW(HWnd, UMsg, WParam, LParam);
            break;
        }
    }
    return 0;
}
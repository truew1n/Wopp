#include <stdio.h>

#include "wave.h"



int32_t ClientWidth;
int32_t ClientHeight;

int32_t BitmapWidth;
int32_t BitmapHeight;

int8_t running = 1;


LRESULT CALLBACK WndProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
DWORD WINAPI SongThread(LPVOID lpParam);


typedef struct song_data_t {
    char filepath[MAX_PATH];
    state_t song_state;
} song_data_t;

HANDLE hsong_mutex;
HANDLE hwave_mutex;
song_data_t song_data = {"", NONE};
HANDLE hsong_thread = {0};
DWORD song_thread_id = {0};


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow)
{
    WNDCLASSW wc = {0};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"Root";
    wc.lpfnWndProc = WndProcedure;

    if(!RegisterClassW(&wc)) return -1;
    
    int32_t Width = 1600;
    int32_t Height = 800;

    RECT window_rect = {0};
    window_rect.right = Width;
    window_rect.bottom = Height;
    window_rect.left = 0;
    window_rect.top = 0;

    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
    HWND window = CreateWindowW(
        wc.lpszClassName,
        L"Wopp Audio Player",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
        NULL, NULL,
        NULL, NULL
    );

    GetWindowRect(window, &window_rect);
    ClientWidth = window_rect.right - window_rect.left;
    ClientHeight = window_rect.bottom - window_rect.top;

    BitmapWidth = Width;
    BitmapHeight = Height;

    uint32_t BytesPerPixel = 4;

    void *memory = VirtualAlloc(
        0,
        BitmapWidth*BitmapHeight*BytesPerPixel,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    );

    BITMAPINFO bitmap_info;
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = BitmapWidth;
    bitmap_info.bmiHeader.biHeight = -BitmapHeight;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(window);

    DragAcceptFiles(window, TRUE);

    hsong_mutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );
    hwave_mutex = CreateMutexA(
        NULL,
        FALSE,
        NULL
    );

    if(hsong_mutex == NULL) {
        fprintf(stderr, "ERROR: Failed to create unnamed mutex!\n");
        exit(-1);
    }

    while(running) {
        MSG msg;
        while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch(msg.message) {
                case WM_DROPFILES: {
                    WaitForSingleObject(hsong_mutex, INFINITE);
                    if(song_data.song_state == PLAYING) {
                        song_data.song_state = FINISHED;
                    }
                    ReleaseMutex(hsong_mutex);
                    WaitForSingleObject(hwave_mutex, INFINITE);
                    HDROP hdrop = (HDROP) msg.wParam;
                    UINT num_files = DragQueryFileA(hdrop, 0xFFFFFFFF, NULL, 0);
                    if(num_files == 1) {
                        DragQueryFileA(hdrop, 0, song_data.filepath, sizeof(song_data.filepath));
                        
                        hsong_thread = CreateThread(NULL, 0, SongThread, &song_data, 0, &song_thread_id);
                    }
                    ReleaseMutex(hwave_mutex);
                    break;
                }
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
            memory, &bitmap_info,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }

    CloseHandle(hsong_mutex);
    return 0;
}

LRESULT CALLBACK WndProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default: {
            return DefWindowProcW(hWnd, msg, wp, lp);
        }
    }
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (uMsg == WOM_DONE) {
        WaitForSingleObject(hsong_mutex, INFINITE);
        state_t *state = ((state_t *) dwInstance);
        if(state != NULL)
            *state = FINISHED;
        ReleaseMutex(hsong_mutex);
    }
}

DWORD WINAPI SongThread(LPVOID lpParam)
{
    WaitForSingleObject(hwave_mutex, INFINITE);
    WaitForSingleObject(hsong_mutex, INFINITE);
    song_data_t *tsong_data = (song_data_t *) lpParam;
    wave_t wave_file = wave_open(tsong_data->filepath);

    int32_t paused = 0;
    WAVEFORMATEX wfx = {0};
    wfx.nSamplesPerSec = wave_file.fmt_chunk.sample_rate;
    wfx.wBitsPerSample = wave_file.fmt_chunk.bits_per_sample;
    wfx.nChannels = wave_file.fmt_chunk.num_channels;
    wfx.wFormatTag = wave_file.fmt_chunk.audio_format;
    wfx.nBlockAlign = wave_file.fmt_chunk.block_align;
    wfx.nAvgBytesPerSec = wave_file.fmt_chunk.byte_rate;
    wfx.cbSize = 0;

    HWAVEOUT hwave_out = NULL;
    MMRESULT result = waveOutOpen(&hwave_out, WAVE_MAPPER, &wfx, (DWORD_PTR) waveOutProc, (DWORD_PTR) tsong_data->song_state, CALLBACK_FUNCTION);
    if(result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: Cannot open waveOut device!\n");
        free(wave_file.data_chunk.data);
        exit(-1);
    }

    WAVEHDR header = {0};
    header.lpData = wave_file.data_chunk.data;
    header.dwBufferLength = wave_file.data_chunk.subchunk_size;
    header.dwBytesRecorded = 0;
    header.dwUser = 0;
    header.dwFlags = 0;
    header.dwLoops = 0;

    result = waveOutPrepareHeader(hwave_out, &header, sizeof(WAVEHDR));
    if(result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: In preparing waveOut header!\n");
        waveOutClose(hwave_out);
        free(wave_file.data_chunk.data);
        exit(-1);
    }
    result = waveOutWrite(hwave_out, &header, sizeof(WAVEHDR));
    if(result != MMSYSERR_NOERROR) {
        fprintf(stderr, "ERROR: While writing to waveOut device!\n");
        waveOutUnprepareHeader(hwave_out, &header, sizeof(WAVEHDR));
        waveOutClose(hwave_out);
        free(wave_file.data_chunk.data);
        exit(-1);
    }

    state_t local_state = PLAYING;
    tsong_data->song_state = PLAYING;
    ReleaseMutex(hwave_mutex);
    while(TRUE) {
        WaitForSingleObject(hsong_mutex, INFINITE);
        if(tsong_data->song_state == FINISHED) {
            break;
        }
        if(tsong_data->song_state != local_state) {
            if(tsong_data->song_state == PAUSED) {
                tsong_data->song_state = PAUSED;
            } else if(tsong_data->song_state == UNPAUSED) {
                tsong_data->song_state = PLAYING;
            }
        }
        local_state = tsong_data->song_state;
        ReleaseMutex(hsong_mutex);
    }
    WaitForSingleObject(hsong_mutex, INFINITE);
    tsong_data->song_state = FINISHED;
    ReleaseMutex(hsong_mutex);

    waveOutReset(hwave_out);
    waveOutUnprepareHeader(hwave_out, &header, sizeof(WAVEHDR));
    waveOutClose(hwave_out);
    
    free(wave_file.data_chunk.data);
    ReleaseMutex(hwave_mutex);
    return 0;
}
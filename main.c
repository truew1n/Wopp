#include <stdio.h>
#include <time.h>
#include <float.h>

#include "wave.h"


int32_t ClientWidth;
int32_t ClientHeight;

int32_t BitmapWidth;
int32_t BitmapHeight;

int8_t running = 1;

LRESULT CALLBACK WndProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
DWORD WINAPI SongThread(LPVOID lpParam);

int64_t map(int64_t x, int64_t in_min, int64_t in_max, int64_t out_min, int64_t out_max);

typedef uint32_t color_t;
void gc_putpixel(void *memory, int32_t x, int32_t y, color_t color);
void gc_fill_screen(void *memory, color_t color);
void gc_fill_rectangle(void *memory, int32_t x, int32_t y, int32_t width, int32_t height, color_t color);
void gc_fill_circle(void *memory, int32_t x, int32_t y, int32_t radius, color_t color);

typedef enum state_t {
    NONE,
    PLAYING,
    PAUSED,
    UNPAUSED,
    FINISHED
} state_t;

typedef struct song_data_t {
    wave_t wave_file;
    state_t song_state;
} song_data_t;

HANDLE hsong_mutex;
HANDLE hwave_mutex;


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
        L"Wopp - Wave Audio Player",
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

    if(!(hsong_mutex && hwave_mutex)) {
        fprintf(stderr, "ERROR: Failed to create unnamed mutexes!\n");
        exit(-1);
    }

    char filepath[MAX_PATH];
    song_data_t song_data = {0, NONE};
    song_data_t local_song_data = {0, NONE};
    HANDLE hsong_thread = {0};
    DWORD song_thread_id = 0;

    // clock_t is time passed in ms
    clock_t clock_start = 0L;
    clock_t delta_time = 0L;
    clock_t visual_delta_sum = 0L;
    clock_t time_delay = 100L;
    clock_t time_passed = 0L;

    MSG msg = {0};
    while(running) {
        clock_start = clock();
        visual_delta_sum += delta_time;

        while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch(msg.message) {
                case WM_DROPFILES: {
                    WaitForSingleObject(hsong_mutex, INFINITE);
                    if(song_data.song_state == PLAYING) {
                        song_data.song_state = FINISHED;
                        local_song_data.song_state = FINISHED;
                        free(local_song_data.wave_file.data_chunk.data);
                    }
                    ReleaseMutex(hsong_mutex);
                    WaitForSingleObject(hwave_mutex, INFINITE);
                    HDROP hdrop = (HDROP) msg.wParam;
                    UINT num_files = DragQueryFileA(hdrop, 0xFFFFFFFF, NULL, 0);
                    if(num_files == 1) {
                        WaitForSingleObject(hsong_mutex, INFINITE);
                        DragQueryFileA(hdrop, 0, filepath, sizeof(filepath));
                        
                        song_data.wave_file = wave_open(filepath);
                        local_song_data.wave_file = wave_open(filepath);
                        local_song_data.song_state = PLAYING;
                        song_data.song_state = NONE;

                        hsong_thread = CreateThread(NULL, 0, SongThread, &song_data, 0, &song_thread_id);
                        ReleaseMutex(hsong_mutex);
                    }
                    ReleaseMutex(hwave_mutex);
                    break;
                }
                case WM_QUIT: {
                    WaitForSingleObject(hsong_mutex, INFINITE);
                    if(song_data.song_state == PLAYING) {
                        song_data.song_state = FINISHED;
                        local_song_data.song_state = FINISHED;
                        free(local_song_data.wave_file.data_chunk.data);
                    }
                    ReleaseMutex(hsong_mutex);
                    running = 0;
                    break;
                }
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if(visual_delta_sum >= time_delay && local_song_data.song_state == PLAYING) {
            gc_fill_screen(memory, 0xFF000000);
            cint8_t *data = local_song_data.wave_file.data_chunk.data;
            uint32_t sample_size = local_song_data.wave_file.fmt_chunk.sample_rate / (CLOCKS_PER_SEC / time_delay);
            uint32_t begin = sample_size * (time_passed / time_delay);
            uint32_t end = begin + sample_size;
            for(uint32_t i = begin; i < end; i += 2) {
                uint16_t value = (data[i] << 8) + data[i + 1];
                int32_t mapped_width = map(i, begin, end, 0, BitmapWidth);
                int32_t mapped_height = map(value, 0, 65535, -(BitmapHeight >> 4), (BitmapHeight >> 4));
                gc_fill_rectangle(memory, mapped_width, BitmapHeight >> 1, (BitmapWidth / sample_size) + 1, -mapped_height, 0xFFFF00FF);
            }
            time_passed += time_delay;
            visual_delta_sum = 0L;
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
        delta_time = (clock() - clock_start);
    }

    CloseHandle(hsong_mutex);
    CloseHandle(hwave_mutex);
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
    return 0;
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
    song_data_t *thread_song_data = (song_data_t *) lpParam;
    wave_t wave_file = thread_song_data->wave_file;

    WAVEFORMATEX wfx = {0};
    wfx.nSamplesPerSec = wave_file.fmt_chunk.sample_rate;
    wfx.wBitsPerSample = wave_file.fmt_chunk.bits_per_sample;
    wfx.nChannels = wave_file.fmt_chunk.num_channels;
    wfx.wFormatTag = wave_file.fmt_chunk.audio_format;
    wfx.nBlockAlign = wave_file.fmt_chunk.block_align;
    wfx.nAvgBytesPerSec = wave_file.fmt_chunk.byte_rate;
    wfx.cbSize = 0;

    HWAVEOUT hwave_out = NULL;
    MMRESULT result = waveOutOpen(&hwave_out, WAVE_MAPPER, &wfx, (DWORD_PTR) waveOutProc, (DWORD_PTR) thread_song_data->song_state, CALLBACK_FUNCTION);
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
    thread_song_data->song_state = PLAYING;
    ReleaseMutex(hsong_mutex);
    while(TRUE) {
        WaitForSingleObject(hsong_mutex, INFINITE);
        if(thread_song_data->song_state == FINISHED) {
            ReleaseMutex(hsong_mutex);
            break;
        }
        if(thread_song_data->song_state != local_state) {
            if(thread_song_data->song_state == PAUSED) {
                thread_song_data->song_state = PAUSED;
            } else if(thread_song_data->song_state == UNPAUSED) {
                thread_song_data->song_state = PLAYING;
            }
        }
        local_state = thread_song_data->song_state;

        ReleaseMutex(hsong_mutex);
    }
    WaitForSingleObject(hsong_mutex, INFINITE);
    thread_song_data->song_state = FINISHED;
    ReleaseMutex(hsong_mutex);

    free(wave_file.data_chunk.data);

    waveOutReset(hwave_out);
    waveOutUnprepareHeader(hwave_out, &header, sizeof(WAVEHDR));
    waveOutClose(hwave_out);
       
    ReleaseMutex(hwave_mutex);
    return 0;
}

int64_t map(int64_t x, int64_t in_min, int64_t in_max, int64_t out_min, int64_t out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void gc_putpixel(void *memory, int32_t x, int32_t y, color_t color)
{
    if((x >= 0 && x < BitmapWidth) && (y >= 0 && y < BitmapHeight))
        *((color_t *) memory + (x + y * BitmapWidth)) = color; 
}

void gc_fill_screen(void *memory, color_t color)
{
    for(int32_t j = 0; j < BitmapHeight; ++j) {
        for(int32_t i = 0; i < BitmapWidth; ++i) {
            gc_putpixel(memory, i, j, color);
        }
    }
}

void gc_fill_rectangle(void *memory, int32_t x, int32_t y, int32_t width, int32_t height, color_t color)
{
    int8_t ws = width < 0;
    int8_t hs = height < 0;
    int32_t x0 = (x + width) * ws + x * !ws,
            y0 = (y + height) * hs + y * !hs,
            x1 = x * ws + (x + width) * !ws,
            y1 = y * hs + (y + height) * !hs;

    for(int32_t j = y0; j < y1; ++j) {
        for(int32_t i = x0; i < x1; ++i) {
            gc_putpixel(memory, i, j, color);
        }
    }
}

void gc_fill_circle(void *memory, int32_t x, int32_t y, int32_t radius, color_t color)
{
    for(int32_t j = -radius; j < radius; ++j) {
        for(int32_t i = -radius; i < radius; ++i) {
            if((j * j + i * i) <= radius * radius) {
                gc_putpixel(memory, x + i, y + j, color);
            }
        }
    }
}
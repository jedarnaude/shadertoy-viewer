#include <windows.h>
#include <time.h>
#include <math.h>

// We do not want to expose this headers in shadertoy, thus the relative include
#include "../../src/glew.h"

#include "ShadertoyTestHelper.h"
#include "XAudioHelper.h"

// These come from having shadertoy.com website on a HD monitor
#define WINDOW_DEFAULT_WIDTH 800
#define WINDOW_DEFAULT_HEIGHT 450
#define CLASS_NAME L"shadertoy_test"

// Windows globals
HGLRC hRC = NULL;
HDC hDC = NULL;
HWND hWnd = NULL;
HINSTANCE hInstance = NULL;

POINT resolution;
BOOL keys[256];
BOOL active = TRUE;

// Audio units (we know we only have SHADERTOY_MAX_CHANNELS + sound pass if exist
DSoundUnit dsound_units[SHADERTOY_MAX_CHANNELS + 1];
int xaudio_units_count = 0;

// Shadertoy globals
ShadertoyInputs inputs = {};
ShadertoyView view = {};
ShadertoyOutputs outputs = {};
ShadertoyConfig config = {};
ShadertoyState state = {};

float GetTimeNow() {
    static LARGE_INTEGER frequency;
    static BOOL has_frenquecy = QueryPerformanceFrequency(&frequency);
    if (has_frenquecy) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (float)now.QuadPart / frequency.QuadPart;
    }
    return 0.0f;
}

void StopAudio(void *sound) {
    DSoundStop((DSoundUnit*)sound);
}

void* InitAudio(int channels, int sample_rate, int bits_per_sample, int buffer_size) {
    dsound_units[xaudio_units_count % 5] = DSoundCreate(channels, sample_rate, bits_per_sample, buffer_size);
    return (void*)&dsound_units[xaudio_units_count++ % 5];
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ImGuiWndProcHandler(hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_ACTIVATE: {
        active = !HIWORD(wParam) ? TRUE : FALSE;
        return 0;
    }
    case WM_CLOSE: {
        PostQuitMessage(0);
        return 0;
    }
    case WM_KEYDOWN: {
        if (wParam < 256) {
            inputs.keyboard.toggle[wParam] = ~inputs.keyboard.toggle[wParam];
            inputs.keyboard.state[wParam] = 0xff;
        }
        return 0;
    }
    case WM_KEYUP: {
        if (wParam < 256) {
            inputs.keyboard.state[wParam] = 0;
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        POINTS points = MAKEPOINTS(lParam);
        inputs.mouse.z = (float)points.x;
        inputs.mouse.w = (float)(inputs.resolution.y - points.y);

        return true;
    }
    case WM_LBUTTONUP: {
        inputs.mouse.z = (float)-fabs(inputs.mouse.z);
        inputs.mouse.w = (float)-fabs(inputs.mouse.w);
        return true;
    }
    case WM_MOUSEMOVE: {
        if (wParam & MK_LBUTTON) {
            POINTS points = MAKEPOINTS(lParam);
            inputs.mouse.x = (float)points.x;
            inputs.mouse.y = (float)(inputs.resolution.y - points.y);
        }
        return true;
    }
    case WM_SIZE: {
        resolution.x = LOWORD(lParam);
        resolution.y = HIWORD(lParam);
        return 0;
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static BOOL 
InitWindow(LPCWSTR title, LONG width, LONG height) {
    hInstance = GetModuleHandle(NULL);

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) {
        return FALSE;
    }

    RECT WindowRect;
    WindowRect.left = 0L;
    WindowRect.top = 0L;
    WindowRect.right = width;
    WindowRect.bottom = height;
    AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

    if (!(hWnd = CreateWindowEx(
        WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
        CLASS_NAME,
        title,
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL,
        NULL,
        hInstance,
        NULL))) {
        return FALSE;
    }

    PIXELFORMATDESCRIPTOR pfd;    
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    if (!(hDC = GetDC(hWnd))) {
        return FALSE;
    }

    int	pixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (!pixelFormat) {
        return FALSE;
    }

    if (!SetPixelFormat(hDC, pixelFormat, &pfd)) {
        return FALSE;
    }

    if (!(hRC = wglCreateContext(hDC))) {
        return FALSE;
    }

    if (!wglMakeCurrent(hDC, hRC)) {
        return FALSE;
    }

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);

    return TRUE;
}

static void 
ShutdownWindow() {
    if (hRC) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hRC);
    }

    if (hDC) {
        ReleaseDC(hWnd, hDC);
    }

    if (hWnd) {
        DestroyWindow(hWnd);
    }

    UnregisterClass(CLASS_NAME, hInstance);
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    MSG		msg;

    if (!InitWindow(L"shadertoy test", WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT)) {
        ShutdownWindow();
        MessageBoxA(0, (char*)"Could not create window/GL context", NULL, MB_OK | MB_ICONERROR);
        return 0;
    }

    // Systems init for shadertoy
    DSoundInit(hWnd);
    
    // GL stuff
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        ShutdownWindow();
        MessageBoxA(0, (char*)"Could not initialize glew", NULL, MB_OK | MB_ICONERROR);
        return 0;
    }

    // Test helper
    ShadertoyTestInfo *test_info = TestInit(GetTimeNow, StopAudio, InitAudio, hWnd);
    ShadertoyTest *test = NULL;

    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // Shadertoy test run
        TestBegin();

        if (test != NULL) {
            // Global time input
            inputs.global_time = GetTimeNow() - test->start_time;

            // Date input
            time_t timestamp = time(NULL);
            struct tm  date;
            date = *localtime(&timestamp);
            inputs.date.x = (float)date.tm_year + 1900;
            inputs.date.y = (float)date.tm_mon;
            inputs.date.z = (float)date.tm_mday;
            inputs.date.w = (float)date.tm_sec + date.tm_min * 60 + date.tm_hour * 60 * 60;

            // Resolution
            view.max.x = resolution.x;
            view.max.y = resolution.y;
            inputs.resolution.x = (float)resolution.x;
            inputs.resolution.y = (float)resolution.y;

            // Mouse and keyboard inputs are processed in the WndProc

            // Render
            ShadertoyRender(&state, &inputs, &view, &outputs);

            // Sound output
            if (outputs.sound_enabled) {
                DSoundUnit *unit = (DSoundUnit*)outputs.sound_data_param;
                DSoundPlay(unit, outputs.sound_buffer_size, (BYTE*)outputs.sound_next_buffer);
                inputs.sound_played_samples = DSoundGetPlayedSamples(unit);
            }
            if (outputs.sound_should_stop) {
                DSoundStop((DSoundUnit*)outputs.sound_data_param);
            }

            // Audio outputs
            for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
                if (outputs.music_enabled_channel[i]) {
                    DSoundUnit *audio = (DSoundUnit*)outputs.music_data_param[i];
                    if (!DSoundIsPlaying(audio)) {
                        int size;
                        BYTE *data;
                        GetAudioInfo(i, &size, (void**)&data);
                        DSoundPlay(audio, size, data);
                    }
                    if (state.music_enabled_channel[i]) {
                        inputs.audio_played_samples[i] = DSoundGetPlayedSamples(audio);
                    }
                }
            }

            GUITestOverlay(true, test, &state, &inputs);
        }

        if (GUITestSelection(true, test_info, &test)) {
            LoadTest(test, &config, &state, &inputs, &outputs);
        }

        TestEnd();

        SwapBuffers(hDC);
    }

    DSoundShutdown();
    TestShutdown();

    return 0;
}
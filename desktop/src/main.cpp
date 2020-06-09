
#include "platform.h"
#include "desktop.h"
#include "input.h"

#include "imgui.h"

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>



#define DEBUG_WINDOW



struct PlatformData
{
    HINSTANCE app_instance;
    HWND window_handle;
    int window_width;
    int window_height;

    FILE *log;


    LARGE_INTEGER previous_time;

    bool want_to_close;
};

static HINSTANCE instance;
static PlatformData *platform_data;





HWND get_window_handle() { return platform_data->window_handle; }

int get_screen_width()   { return platform_data->window_width;  }
int get_screen_height()  { return platform_data->window_height; }
float get_aspect_ratio() { return (float)platform_data->window_width / platform_data->window_height; }
HDC get_device_context() { return GetDC(platform_data->window_handle); }

bool want_to_close() { return platform_data->want_to_close; }





extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam)) return true;

    LRESULT result = 0; 

    switch (message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        break;

        case WM_CLOSE: 
        {
            DestroyWindow(window);
            return 0;
        }  
        break;

        /*
        case WM_PAINT:
        {
        ValidateRect(window_handle, 0);
        }
        break;
        */
 
        case WM_KEYDOWN:
        {
            if(wParam == VK_ESCAPE)
            {
                platform_data->want_to_close = true;
            }
            record_key_event(wParam, true);
        }
        break;
        case WM_KEYUP:   { record_key_event(wParam, false); break; }

        case WM_LBUTTONDOWN: { record_mouse_event(0, true);  break; }
        case WM_LBUTTONUP:   { record_mouse_event(0, false); break; }

        // idk
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_SETFOCUS:
        case WM_WINDOWPOSCHANGING:
        {
#ifndef DEBUG_WINDOW
            result = SetWindowPos(platform_data->window_handle, HWND_BOTTOM, 0, 0, platform_data->window_width, platform_data->window_height, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
#endif

            // result = SetWindowPos(platform_data->window_handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
        }
        break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;
    }

    return result;
}


void init_platform()
{
    platform_data = (PlatformData *)calloc(1, sizeof(PlatformData));

    platform_data->app_instance = instance;

    // Create the window class
    WNDCLASS window_class = {};

    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = platform_data->app_instance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = "Windows Program Class";

    if(!RegisterClass(&window_class)) { return; }

    RECT rect;
    BOOL result = SystemParametersInfoA(SPI_GETWORKAREA, 0, &rect, 0);
    unsigned window_width = rect.right;
    unsigned window_height = rect.bottom;

    // Create the window
#ifdef DEBUG_WINDOW
    platform_data->window_handle = CreateWindowEx(0,                  // Extended style
      window_class.lpszClassName,        // Class name
      "First",                           // Window name
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // Style of the window
      0,                                 // Initial X position
      0,                                 // Initial Y position
      window_width,                     // Initial width
      window_height,                    // Initial height
      0,                                 // Handle to the window parent
      0,                                 // Handle to a menu
      platform_data->app_instance,        // Handle to an instance
      0);
#else
    platform_data->window_handle = CreateWindowEx(0,   // Extended style
        window_class.lpszClassName,        // Class name
        "First",                           // Window name
        WS_POPUP | WS_VISIBLE,             // Style of the window
        0,                                 // Initial X position
        0,                                 // Initial Y position
        window_width,                      // Initial width
        window_height,                     // Initial height 
        0,                                 // Handle to the window parent
        0,                                 // Handle to a menu
        platform_data->app_instance,       // Handle to an instance
        0);                                // Pointer to a CLIENTCTREATESTRUCT
#endif

    /*

    This is a transparent window
        platform_data->window_handle = CreateWindowEx(WS_EX_COMPOSITED,   // Extended style
        window_class.lpszClassName,        // Class name
        "First",                           // Window name
        WS_POPUP | WS_VISIBLE,             // Style of the window
        0,                                 // Initial X position
        0,                                 // Initial Y position
        window_width,                      // Initial width
        window_height,                     // Initial height
        0,                                 // Handle to the window parent
        0,                                 // Handle to a menu
        platform_data->app_instance,       // Handle to an instance
        0);                                // Pointer to a CLIENTCTREATESTRUCT

    result = SetWindowLong(platform_data->window_handle, GWL_EXSTYLE, WS_EX_LAYERED) ;
    result = SetLayeredWindowAttributes(platform_data->window_handle, RGB(0, 0, 0), 10, LWA_COLORKEY);

    */

    if(!platform_data->window_handle) { return; }


    RECT client_rect;
    BOOL success = GetClientRect(platform_data->window_handle, &client_rect);

    platform_data->window_width = client_rect.right;
    platform_data->window_height = client_rect.bottom;

#ifndef DEBUG_WINDOW
    result = SetWindowPos(platform_data->window_handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
#endif

    QueryPerformanceCounter(&platform_data->previous_time);

    platform_data->log = fopen("output/log.txt", "w");
}

void platform_events()
{
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        if(message.message == WM_QUIT)
        {
            platform_data->want_to_close = true;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}


// Get dt in seconds
float get_dt()
{
    LARGE_INTEGER counts_per_second;
    QueryPerformanceFrequency(&counts_per_second);

    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);

    LONGLONG counts_passed = current_time.QuadPart - platform_data->previous_time.QuadPart;

    platform_data->previous_time = current_time;

    double dt = (double)counts_passed / counts_per_second.QuadPart;

    return (float)dt;
}

v2 mouse_screen_position()
{
  POINT window_client_pos;
  BOOL success = GetCursorPos(&window_client_pos);
  success = ScreenToClient(get_window_handle(), &window_client_pos);

  return v2(window_client_pos.x, window_client_pos.y);
}

char *read_file_as_string(const char *path)
{
    FILE *file = fopen(path, "rb");
    if(file == nullptr) return nullptr;

    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char *buffer = (char *)malloc(size + 1);

    fread(buffer, size, 1, file);

    buffer[size] = '\0';

    return buffer;
}


void log_error_fn(const char *file, int line, const char *format, ...)
{
    fprintf(platform_data->log, "-- ERROR : ");
    fprintf(platform_data->log, "%s : %d\n", file, line);

    va_list args;
    va_start(args, format);
    vfprintf(platform_data->log, format, args);
    va_end(args);

    fprintf(platform_data->log, "\n");

    fflush(platform_data->log);
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    instance = hInstance;
    start_desktop();
}


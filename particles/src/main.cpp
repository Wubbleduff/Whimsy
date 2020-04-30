
#include "platform.h"
#include "particles.h"
#include "input.h"

#include "imgui.h"

#include <windows.h>


struct PlatformData
{
    HINSTANCE app_instance;
    HWND window_handle;
    int window_width;
    int window_height;

    LARGE_INTEGER previous_time;

    bool want_to_close;
};

static HINSTANCE instance;
static PlatformData *platform_data;





HWND get_window_handle() { return platform_data->window_handle; }

int get_window_width()   { return platform_data->window_width;  }
int get_window_height()  { return platform_data->window_height; }

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
            record_key_event(wParam, false);
        }
        break;
        case WM_KEYUP:   { record_key_event(wParam, false); break; }

        case WM_LBUTTONDOWN: { record_mouse_event(0, true);  break; }
        case WM_LBUTTONUP:   { record_mouse_event(0, false); break; }

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



#define TRANSPARENT_WINDOWx

    // Create the window
#ifdef TRANSPARENT_WINDOW
    //unsigned window_width = GetSystemMetrics(SM_CXSCREEN);
    //unsigned window_width = GetSystemMetrics(SM_CYSCREEN);
    unsigned window_width = 1024;
    unsigned window_height = 768;
    platform_data->window_handle = CreateWindowEx(WS_EX_COMPOSITED,   // Extended style
            window_class.lpszClassName,        // Class name
            "First",                           // Window name
            WS_POPUP | WS_VISIBLE,             // Style of the window
            0,                                 // Initial X position
            0,                                 // Initial Y position
            window_width,                     // Initial width
            window_height,                    // Initial height 
            0,                                 // Handle to the window parent
            0,                                 // Handle to a menu
            platform_data.app_instance,        // Handle to an instance
            0);                                // Pointer to a CLIENTCTREATESTRUCT

    BOOL result = SetWindowLong(platform_data->window_handle, GWL_EXSTYLE, WS_EX_LAYERED) ;
    result = SetLayeredWindowAttributes(platform_data->window_handle, RGB(0, 0, 0), 10, LWA_COLORKEY);
#else
    unsigned width = 1920;
    float ratio = 16.0f / 9.0f;
    unsigned window_width = width;
    unsigned window_height = (unsigned)((1.0f / ratio) * width);
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
            0);                                // Pointer to a CLIENTCTREATESTRUCT
#endif
    if(!platform_data->window_handle) { return; }


    RECT client_rect;
    BOOL success = GetClientRect(platform_data->window_handle, &client_rect);

    platform_data->window_width = client_rect.right;
    platform_data->window_height = client_rect.bottom;

    QueryPerformanceCounter(&platform_data->previous_time);
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

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  instance = hInstance;
  start_particles();
}


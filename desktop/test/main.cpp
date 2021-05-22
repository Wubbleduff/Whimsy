
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>



struct PlatformData
{
    HINSTANCE app_instance;
    HWND window_handle;
    int window_width;
    int window_height;

    bool want_to_close;

    FILE *log;
};


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    HWND p = FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL);
    HWND* ret = (HWND*)lParam;

    if (p)
    {
        // Gets the WorkerW Window after the current one.
        *ret = FindWindowEx(NULL, hwnd, "WorkerW", NULL);
    }
    return true;
}
HWND get_wallpaper_window()
{
    // Fetch the Progman window
    HWND progman = FindWindow("ProgMan", NULL);
    // Send 0x052C to Progman. This message directs Progman to spawn a 
    // WorkerW behind the desktop icons. If it is already there, nothing 
    // happens.
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);
    // We enumerate all Windows, until we find one, that has the SHELLDLL_DefView 
    // as a child. 
    // If we found that window, we take its next sibling and assign it to workerw.
    HWND wallpaper_hwnd = nullptr;
    EnumWindows(EnumWindowsProc, (LPARAM)&wallpaper_hwnd);
    // Return the handle you're looking for.
    return wallpaper_hwnd;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
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

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;
    }

    return result;
}

void platform_events(PlatformData *platform_data)
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

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    PlatformData *platform_data = (PlatformData *)calloc(1, sizeof(PlatformData));

    platform_data->log = fopen("log.txt", "wt");

    platform_data->app_instance = hInstance;


    // Create the window class
    WNDCLASS window_class = {};

    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = platform_data->app_instance;
    //window_class.hIcon = icon;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = "Windows Program Class";

    if(!RegisterClass(&window_class)) { return 0; }


#define WALLPAPER 1
#if WALLPAPER
    platform_data->window_handle = get_wallpaper_window();
#else
    platform_data->window_handle = CreateWindowEx(0,                                // Extended style
            window_class.lpszClassName,        // Class name
            "asdf",                            // Window name
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // Style of the window
            CW_USEDEFAULT,                     // Initial X position
            CW_USEDEFAULT,                     // Initial Y position
            CW_USEDEFAULT,                     // Initial width
            CW_USEDEFAULT,                     // Initial height 
            0,                                 // Handle to the window parent
            0,                                 // Handle to a menu
            hInstance,                         // Handle to an instance
            0);                                // Pointer to a CLIENTCTREATESTRUCT
#endif


    RECT client_rect;
    BOOL success = GetClientRect(platform_data->window_handle, &client_rect);

    platform_data->window_width = client_rect.right;
    platform_data->window_height = client_rect.bottom;






    unsigned *frame_buffer;
    HDC DIB_handle;
    {
        static HBITMAP DIB_bitmap;
        static int DIB_width;
        static int DIB_height;
        static int DIB_row_byte_width;
        HDC hdc = GetDC(platform_data->window_handle);
        RECT client_rect;
        GetClientRect(platform_data->window_handle, &client_rect);
        DIB_width = client_rect.right;
        DIB_height = client_rect.bottom;
        int bitCount = 32;
        DIB_row_byte_width = ((DIB_width * (bitCount / 8) + 3) & -4);
        int totalBytes = DIB_row_byte_width * DIB_height;
        BITMAPINFO mybmi = {};
        mybmi.bmiHeader.biSize = sizeof(mybmi);
        mybmi.bmiHeader.biWidth = DIB_width;
        mybmi.bmiHeader.biHeight = DIB_height;
        mybmi.bmiHeader.biPlanes = 1;
        mybmi.bmiHeader.biBitCount = bitCount;
        mybmi.bmiHeader.biCompression = BI_RGB;
        mybmi.bmiHeader.biSizeImage = totalBytes;
        mybmi.bmiHeader.biXPelsPerMeter = 0;
        mybmi.bmiHeader.biYPelsPerMeter = 0;
        DIB_handle = CreateCompatibleDC(hdc);
        DIB_bitmap = CreateDIBSection(hdc, &mybmi, DIB_RGB_COLORS, (VOID **)&frame_buffer, NULL, 0);
        (HBITMAP)SelectObject(DIB_handle, DIB_bitmap);

        ReleaseDC(platform_data->window_handle, hdc);
    }









    platform_data->want_to_close = false;
    while(!platform_data->want_to_close)
    {
        platform_events(platform_data);

        memset(frame_buffer, 0, platform_data->window_width * platform_data->window_height * 4);

#if 0
        for(int y = 0; y < platform_data->window_height; y += 100)
        {
            for(int x = 0; x < platform_data->window_width; x += 100)
            {
                frame_buffer[y * platform_data->window_width + y] = 0xFFFFFFFF;
            }
        }
#endif
        static int start = 0;
        start += 10;
        if(start > 200) start = 0;
        for(int y = platform_data->window_height - 1 - start; y >= 500 + start; y -= 1)
        //for(int y = 50; y < platform_data->window_height; y++)
        {
            //for(int x = 0; x < platform_data->window_width; x += 1)
            {
                frame_buffer[y * platform_data->window_width + (platform_data->window_height - 1 + 1920) - y] = 0xFFF00FFF;
            }
        }



        HDC hdc = GetDC(platform_data->window_handle);
        BitBlt(hdc, 0, 0, platform_data->window_width, platform_data->window_height, DIB_handle, 0, 0, SRCCOPY);

        Sleep(16);
    }


    return 0;
}


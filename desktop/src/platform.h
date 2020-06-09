
#pragma once

#include "my_math.h"

#include <windows.h>

void init_platform();

void platform_events();

bool want_to_close();

HWND get_window_handle();
HDC get_device_context();
int get_screen_width();
int get_screen_height();
float get_aspect_ratio();
float get_dt();

v2 mouse_screen_position();



char *read_file_as_string(const char *path);



#define log_error(format, ...) log_error_fn(__FILE__, __LINE__, format, __VA_ARGS__);
void log_error_fn(const char *file, int line, const char *format, ...);



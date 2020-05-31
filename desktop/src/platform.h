
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


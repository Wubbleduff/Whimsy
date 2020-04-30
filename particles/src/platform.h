
#pragma once

#include "my_math.h"

#include <windows.h>

void init_platform();

void platform_events();

bool want_to_close();

HWND get_window_handle();
int get_window_width();
int get_window_height();
float get_dt();

v2 mouse_screen_position();


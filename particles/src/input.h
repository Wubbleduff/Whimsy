
#pragma once


#include "my_math.h"

bool key_toggled_down(int button);
bool key_state(int button);

bool mouse_toggled_down(int button);
bool mouse_state(int button);




void init_input();
void read_input();
void record_key_event(int vk_code, bool state);
void record_mouse_event(int vk_code, bool state);



#include "input.h"

#include <windows.h>
#include <assert.h>

static const int MAX_KEYS = 256;
static const int MAX_MOUSE_KEYS = 8;
struct InputData
{
  bool current_key_states[MAX_KEYS];
  bool previous_key_states[MAX_KEYS];
  bool event_key_states[MAX_KEYS]; // For recording windows key press events

  bool current_mouse_states[MAX_MOUSE_KEYS];
  bool previous_mouse_states[MAX_MOUSE_KEYS];
  bool event_mouse_states[MAX_MOUSE_KEYS]; // For recording windows key press events
};
static InputData *input_data;





// For recording windows key press events
void record_key_event(int vk_code, bool state)
{
  input_data->event_key_states[vk_code] = state;
}

// For recording windows key press events
void record_mouse_event(int vk_code, bool state)
{
  input_data->event_mouse_states[vk_code] = state;
}


void read_input()
{
  // Read keyboard input
  memcpy(input_data->previous_key_states, input_data->current_key_states, sizeof(bool) * MAX_KEYS);
  memcpy(input_data->current_key_states, input_data->event_key_states, sizeof(bool) * MAX_KEYS);

  // Read mouse input
  memcpy(input_data->previous_mouse_states, input_data->current_mouse_states, sizeof(bool) * MAX_MOUSE_KEYS);
  memcpy(input_data->current_mouse_states, input_data->event_mouse_states, sizeof(bool) * MAX_MOUSE_KEYS);
}


bool key_toggled_down(int key)
{
  assert(key < MAX_KEYS);

  return (input_data->previous_key_states[key] == false && input_data->current_key_states[key] == true) ? true : false;
}

bool key_state(int key)
{
  assert(key < MAX_KEYS);

  return input_data->current_key_states[key];
}


bool mouse_toggled_down(int key)
{
  assert(key < MAX_MOUSE_KEYS);

  return (input_data->previous_mouse_states[key] == false && input_data->current_mouse_states[key] == true) ? true : false;
}

bool mouse_state(int key)
{
  assert(key < MAX_MOUSE_KEYS);

  return input_data->current_mouse_states[key];
}


void init_input()
{
  input_data = (InputData *)malloc(sizeof(InputData));
  memset(input_data->current_key_states,  0, sizeof(bool) * MAX_KEYS);
  memset(input_data->previous_key_states, 0, sizeof(bool) * MAX_KEYS);
  memset(input_data->event_key_states,    0, sizeof(bool) * MAX_KEYS);

  memset(input_data->current_mouse_states,  0, sizeof(bool) * MAX_MOUSE_KEYS);
  memset(input_data->previous_mouse_states, 0, sizeof(bool) * MAX_MOUSE_KEYS);
  memset(input_data->event_mouse_states,    0, sizeof(bool) * MAX_MOUSE_KEYS);
}


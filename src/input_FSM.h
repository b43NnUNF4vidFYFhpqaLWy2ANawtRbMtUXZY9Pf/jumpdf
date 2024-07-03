#pragma once

#include "win.h"
#include "viewer.h"

typedef enum {
    STATE_NORMAL = 0,
    STATE_g, // g pressed once
    STATE_REPEAT, // 0-9
} InputState;

typedef InputState (*input_state_func)(Window*, guint);

InputState on_state_normal(Window* window, guint keyval);
InputState on_state_g(Window* window, guint keyval);
InputState on_state_repeat(Window* window, guint keyval);
InputState execute_state(InputState current_state, Window* window, guint keyval);
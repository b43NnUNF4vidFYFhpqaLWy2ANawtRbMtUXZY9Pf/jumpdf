#pragma once

#include "win.h"
#include "viewer.h"

typedef enum {
    STATE_NORMAL,
    STATE_g, // g pressed once
} InputState;

typedef InputState (*input_state_func)(Viewer*, guint);

InputState on_state_normal(Viewer* viewer, guint keyval);
InputState on_state_g(Viewer* viewer, guint keyval);

InputState execute_state(InputState current_state, Viewer* viewer, guint keyval);
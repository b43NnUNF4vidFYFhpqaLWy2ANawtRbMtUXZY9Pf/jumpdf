#pragma once

#include "win.h"
#include "viewer.h"

typedef enum {
    STATE_NORMAL = 0,
    STATE_g, // g pressed once
    STATE_NUMBER, // 0-9
    STATE_FOLLOW_LINKS,
    STATE_TOC_FOCUS,
    STATE_MARK,
} InputState;

typedef InputState (*input_state_func)(Window*, guint);

InputState on_state_normal(Window* window, guint keyval);
InputState on_state_g(Window* window, guint keyval);
InputState on_state_number(Window* window, guint keyval);
InputState on_state_follow_links(Window* window, guint keyval);
InputState on_state_toc_focus(Window* window, guint keyval);
InputState on_state_mark(Window* window, guint keyval);

InputState execute_state(InputState current_state, Window* window, guint keyval);
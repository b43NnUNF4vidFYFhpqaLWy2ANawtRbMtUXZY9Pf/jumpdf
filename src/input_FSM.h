#pragma once

#include "window.h"
#include "viewer.h"

typedef enum {
    STATE_NORMAL = 0,
    STATE_g,
    STATE_NUMBER,
    STATE_FOLLOW_LINKS,
    STATE_TOC_FOCUS,
    STATE_GROUP_CLEAR,
    STATE_GROUP_SWAP,
    STATE_GROUP_OVERWRITE,
    STATE_MARK,
    STATE_MARK_CLEAR,
    STATE_MARK_SWAP,
    STATE_MARK_OVERWRITE,
} InputState;

typedef InputState(*input_state_func)(Window *, guint);

InputState on_state_normal(Window *window, guint keyval);
InputState on_state_g(Window *window, guint keyval);
InputState on_state_number(Window *window, guint keyval);
InputState on_state_follow_links(Window *window, guint keyval);
InputState on_state_toc_focus(Window *window, guint keyval);
InputState on_state_group_clear(Window *window, guint keyval);
InputState on_state_group_swap(Window *window, guint keyval);
InputState on_state_group_overwrite(Window *window, guint keyval);
InputState on_state_mark(Window *window, guint keyval);
InputState on_state_mark_clear(Window *window, guint keyval);
InputState on_state_mark_swap(Window *window, guint keyval);
InputState on_state_mark_overwrite(Window *window, guint keyval);

InputState execute_state(InputState current_state, Window *window, guint keyval);
#include "input_FSM.h"
#include "config.h"
#include "keys.h"

static InputState execute_command(Window* window, guint keyval, unsigned int repeat_count);

input_state_func input_state_funcs[] = {
    on_state_normal,
    on_state_g,
    on_state_repeat,
};

InputState on_state_normal(Window* window, guint keyval) {
    InputState next_state = STATE_NORMAL;
    Viewer* viewer = window_get_viewer(window);
    
    if (keyval >= KEY_1 && keyval <= KEY_9) {
        viewer->repeat_count = keyval - KEY_0;
        next_state = STATE_REPEAT;
    } else {
        switch (keyval) {
            case KEY_0:
                viewer_set_scale(viewer, 1.0);
                break;
            case KEY_c:
                viewer_toggle_center_mode(viewer);
                break;
            case KEY_s:
                viewer_fit_horizontal(viewer);
                break;
            case KEY_v:
                viewer_fit_vertical(viewer);
                break;
            case KEY_g:
                next_state = STATE_g;
                break;
            case KEY_G:
                viewer->current_page = viewer->n_pages - 1;
                viewer->y_offset = STEPS - 1;
                break;
            case KEY_SLASH:
                window_show_search_dialog(window);
                break;
            default:
                next_state = execute_command(window, keyval, 1);
                break;
        }
    }

    return next_state;
}

InputState on_state_g(Window* window, guint keyval) {
    InputState next_state = STATE_NORMAL;
    Viewer* viewer = window_get_viewer(window);

    switch (keyval) {
        case KEY_g:
            viewer->current_page = 0;
            viewer->y_offset = 0;
            break;
    }

    return next_state;
}

InputState on_state_repeat(Window* window, guint keyval) {
    InputState next_state;
    Viewer* viewer = window_get_viewer(window);

    if (keyval >= KEY_0 && keyval <= KEY_9) {
        viewer->repeat_digits++;
        viewer->repeat_count = viewer->repeat_count * 10 + (keyval - KEY_0);
        next_state = STATE_REPEAT;
    } else {
        next_state = execute_command(window, keyval, viewer->repeat_count);
        viewer->repeat_count = 0;
        viewer->repeat_digits = 0;
    }

    return next_state;
}

InputState execute_state(InputState current_state, Window* window, guint keyval) {
    return input_state_funcs[current_state](window, keyval);
}

static InputState execute_command(Window* window, guint keyval, unsigned int repeat_count) {
    InputState next_state = STATE_NORMAL;
    Viewer* viewer = window_get_viewer(window);

    for (unsigned int i = 0; i < repeat_count; i++) {
        switch (keyval) {
            case KEY_PLUS:
                viewer_set_scale(viewer, viewer->scale + SCALE_STEP);
                break;
            case KEY_MINUS:
                viewer_set_scale(viewer, viewer->scale - SCALE_STEP);
                break;
            case KEY_u:
                viewer->y_offset -= STEPS / 2.0;
                break;
            case KEY_d:
                viewer->y_offset += STEPS / 2.0;
                break;
            case KEY_h:
                viewer->x_offset++;
                break;
            case KEY_j:
                viewer->y_offset++;
                break;
            case KEY_k:
                viewer->y_offset--;
                break;
            case KEY_l:
                viewer->x_offset--;
                break;
            case KEY_n:
                viewer_goto_next_search(viewer);
                break;
            case KEY_N:
                viewer_goto_prev_search(viewer);
                break;
        }
    }

    return next_state;
}
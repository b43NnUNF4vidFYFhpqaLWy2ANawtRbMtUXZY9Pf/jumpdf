#include "input_FSM.h"
#include "config.h"
#include "keys.h"

input_state_func input_state_funcs[] = {
    on_state_normal,
    on_state_g,
};

InputState on_state_normal(Window* win, guint keyval) {
    InputState next_state = STATE_NORMAL;
    Viewer* viewer = window_get_viewer(win);

    switch (keyval) {
        case KEY_PLUS:
            viewer_set_scale(viewer, viewer->scale + SCALE_STEP);
            break;
        case KEY_MINUS:
            viewer_set_scale(viewer, viewer->scale - SCALE_STEP);
            break;
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
        case KEY_g:
            next_state = STATE_g;
            break;
        case KEY_G:
            viewer->current_page = viewer->n_pages - 1;
            viewer->y_offset = STEPS - 1;
            break;
        case KEY_SLASH:
            window_show_search_dialog(win);
            break;
    }

    return next_state;
}

InputState on_state_g(Window* win, guint keyval) {
    InputState next_state = STATE_NORMAL;
    Viewer* viewer = window_get_viewer(win);

    switch (keyval) {
        case KEY_g:
            viewer->current_page = 0;
            viewer->y_offset = 0;
            break;
    }

    return next_state;
}

InputState execute_state(InputState current_state, Window* win, guint keyval) {
    return input_state_funcs[current_state](win, keyval);
}
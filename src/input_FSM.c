#include "input_FSM.h"
#include "config.h"
#include "keys.h"

static InputState execute_command(Window* window, guint keyval, unsigned int repeat_count);

input_state_func input_state_funcs[] = {
    on_state_normal,
    on_state_g,
    on_state_number,
    on_state_follow_links,
};

InputState on_state_normal(Window* window, guint keyval) {
    InputState next_state = STATE_NORMAL;
    Viewer* viewer = window_get_viewer(window);
    
    if (keyval >= KEY_1 && keyval <= KEY_9) {
        viewer->input_number = keyval - KEY_0;
        next_state = STATE_NUMBER;
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
            case KEY_a:
                viewer_fit_vertical(viewer);
                break;
            case KEY_g:
                next_state = STATE_g;
                break;
            case KEY_G:
                viewer->current_page = viewer->n_pages - 1;
                viewer->y_offset = STEPS - 1;
                break;
            case KEY_f:
                viewer->follow_links_mode = true;
                viewer->link_index_input = 0;
                next_state = STATE_FOLLOW_LINKS;
                break;
            case KEY_TAB:
                window_toggle_toc(window);
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

InputState on_state_number(Window* window, guint keyval) {
    InputState next_state;
    Viewer* viewer = window_get_viewer(window);

    if (keyval >= KEY_0 && keyval <= KEY_9) {
        viewer->input_number = viewer->input_number * 10 + (keyval - KEY_0);
        next_state = STATE_NUMBER;
    } else if (keyval == KEY_G) {
        viewer->current_page = viewer->input_number - 1;
        viewer->y_offset = 0;
        viewer_fit_vertical(viewer);
        viewer->input_number = 0;
        next_state = STATE_NORMAL;
    } else {
        next_state = execute_command(window, keyval, viewer->input_number);
        viewer->input_number = 0;
    }

    return next_state;
}

InputState on_state_follow_links(Window* window, guint keyval) {
    InputState next_state;
    Viewer* viewer = window_get_viewer(window);
    PopplerLinkMapping *link_mapping;
    PopplerActionUri *action_uri;
    GError *error = NULL;
    unsigned int page_num;
    PopplerDest *dest;

    if (keyval >= KEY_0 && keyval <= KEY_9) {
        viewer->link_index_input = viewer->link_index_input * 10 + (keyval - KEY_0);
        next_state = STATE_FOLLOW_LINKS;
    } else if (keyval == KEY_ENTER && viewer->link_index_input - 1 < viewer->visible_links->len) {
        link_mapping = g_ptr_array_index(viewer->visible_links, viewer->link_index_input - 1);

        switch (link_mapping->action->type) {
            case POPPLER_ACTION_URI:
                action_uri = (PopplerActionUri *)link_mapping->action;
                g_app_info_launch_default_for_uri(action_uri->uri, NULL, &error);
                if (error != NULL) {
                    g_printerr("Poppler: Error launching URI: %s\n", error->message);
                    g_error_free(error);
                }
                break;
            case POPPLER_ACTION_GOTO_DEST:
                if (link_mapping->action->goto_dest.dest->type == POPPLER_DEST_NAMED) {
                    dest = poppler_document_find_dest(viewer->doc, link_mapping->action->goto_dest.dest->named_dest);
                    page_num = dest->page_num - 1;
                    poppler_dest_free(dest);
                } else {
                    page_num = link_mapping->action->goto_dest.dest->page_num - 1;
                }

                viewer->current_page = page_num;
                viewer->y_offset = 0;
                viewer_fit_vertical(viewer);
                break;
            default:
                g_printerr("Poppler: Unsupported link type\n");
                break;
        }

        viewer->follow_links_mode = false;
        next_state = STATE_NORMAL;
    } else {
        viewer->follow_links_mode = false;
        next_state = STATE_NORMAL;
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
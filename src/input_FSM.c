#include "input_FSM.h"
#include "config.h"

static InputState execute_command(Window* window, guint keyval, unsigned int repeat_count);

input_state_func input_state_funcs[] = {
    on_state_normal,
    on_state_g,
    on_state_number,
    on_state_follow_links,
    on_state_toc_focus,
};

InputState on_state_normal(Window* window, guint keyval) {
    InputState next_state = STATE_NORMAL;
    Viewer* viewer = window_get_viewer(window);
    
    if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_9) {
        viewer->input_number = keyval - GDK_KEY_0;
        next_state = STATE_NUMBER;
    } else {
        switch (keyval) {
            case GDK_KEY_0:
                viewer_set_scale(viewer, 1.0);
                break;
            case GDK_KEY_c:
                viewer_toggle_center_mode(viewer);
                break;
            case GDK_KEY_s:
                viewer_fit_horizontal(viewer);
                break;
            case GDK_KEY_a:
                viewer_fit_vertical(viewer);
                break;
            case GDK_KEY_g:
                next_state = STATE_g;
                break;
            case GDK_KEY_G:
                viewer->current_page = viewer->n_pages - 1;
                viewer->y_offset = STEPS - 1;
                break;
            case GDK_KEY_f:
                viewer->follow_links_mode = true;
                viewer->link_index_input = 0;
                next_state = STATE_FOLLOW_LINKS;
                break;
            case GDK_KEY_Tab:
                window_toggle_toc(window);
                next_state = STATE_TOC_FOCUS;
                break;
            case GDK_KEY_slash:
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
        case GDK_KEY_g:
            viewer->current_page = 0;
            viewer->y_offset = 0;
            break;
    }

    return next_state;
}

InputState on_state_number(Window* window, guint keyval) {
    InputState next_state;
    Viewer* viewer = window_get_viewer(window);

    if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9) {
        viewer->input_number = viewer->input_number * 10 + (keyval - GDK_KEY_0);
        next_state = STATE_NUMBER;
    } else if (keyval == GDK_KEY_G) {
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

    if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9) {
        viewer->link_index_input = viewer->link_index_input * 10 + (keyval - GDK_KEY_0);
        next_state = STATE_FOLLOW_LINKS;
    } else if (keyval == GDK_KEY_Return && viewer->link_index_input - 1 < viewer->visible_links->len) {
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

InputState on_state_toc_focus(Window* window, guint keyval) {
    InputState next_state;
    GtkListBox *toc_list_box = window_get_toc_listbox(window);
    GtkListBoxRow *current_row = gtk_list_box_get_selected_row(toc_list_box);
    int current_index;
    GtkListBoxRow *new_row = NULL;

    if (current_row != NULL) {
        current_index = gtk_list_box_row_get_index(current_row);
    }

    switch (keyval) {
        case GDK_KEY_Tab:
            window_toggle_toc(window);
            next_state = STATE_NORMAL;
            break;
        case GDK_KEY_j:
            if (current_row != NULL) {
                new_row = gtk_list_box_get_row_at_index(toc_list_box, current_index + 1);
                while (new_row != NULL && !gtk_widget_get_visible(GTK_WIDGET(new_row))) {
                    new_row = gtk_list_box_get_row_at_index(toc_list_box, gtk_list_box_row_get_index(new_row) + 1);
                }
            }
            next_state = STATE_TOC_FOCUS;
            break;
        case GDK_KEY_k:
            if (current_row != NULL) {
                new_row = gtk_list_box_get_row_at_index(toc_list_box, current_index - 1);
                while (new_row != NULL && !gtk_widget_get_visible(GTK_WIDGET(new_row))) {
                    new_row = gtk_list_box_get_row_at_index(toc_list_box, gtk_list_box_row_get_index(new_row) - 1);
                }
            }
            next_state = STATE_TOC_FOCUS;
            break;
        case GDK_KEY_Return:
            window_execute_toc_row(window, current_row);
            next_state = STATE_TOC_FOCUS;
            break;
        case GDK_KEY_slash:
            window_focus_toc_search(window);
            next_state = STATE_TOC_FOCUS;
            break;
        default:
            next_state = STATE_TOC_FOCUS;
            break;
    }

    if (new_row != NULL) {
        gtk_list_box_select_row(toc_list_box, new_row);
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
            case GDK_KEY_plus:
                viewer_set_scale(viewer, viewer->scale + SCALE_STEP);
                break;
            case GDK_KEY_minus:
                viewer_set_scale(viewer, viewer->scale - SCALE_STEP);
                break;
            case GDK_KEY_u:
                viewer->y_offset -= STEPS / 2.0;
                break;
            case GDK_KEY_d:
                viewer->y_offset += STEPS / 2.0;
                break;
            case GDK_KEY_h:
                viewer->x_offset++;
                break;
            case GDK_KEY_j:
                viewer->y_offset++;
                break;
            case GDK_KEY_k:
                viewer->y_offset--;
                break;
            case GDK_KEY_l:
                viewer->x_offset--;
                break;
            case GDK_KEY_n:
                viewer_goto_next_search(viewer);
                break;
            case GDK_KEY_N:
                viewer_goto_prev_search(viewer);
                break;
        }
    }

    return next_state;
}
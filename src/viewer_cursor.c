#include <stdlib.h>

#include "viewer_cursor.h"
#include "config.h"

ViewerCursor *viewer_cursor_new(ViewerInfo *info) {
    ViewerCursor *cursor = malloc(sizeof(ViewerCursor));
    if (cursor == NULL) {
        return NULL;
    }

    viewer_cursor_init(cursor, info);
    
    return cursor;
}

void viewer_cursor_init(ViewerCursor *cursor, ViewerInfo *info) {
    cursor->info = info;
    
    cursor->current_page = 0;
    cursor->x_offset = 0.0;
    cursor->y_offset = 0.0;
    cursor->scale = 1.0;
    cursor->center_mode = true;
}

void viewer_cursor_destroy(ViewerCursor *cursor) {}

void viewer_cursor_fit_horizontal(ViewerCursor *cursor) {
    cursor->scale = cursor->info->view_width / cursor->info->pdf_width;
    viewer_cursor_center(cursor);
}

void viewer_cursor_fit_vertical(ViewerCursor *cursor) {
    cursor->scale = cursor->info->view_height / cursor->info->pdf_height;
    viewer_cursor_center(cursor);
}

void viewer_cursor_toggle_center_mode(ViewerCursor *cursor) {
    cursor->center_mode = !cursor->center_mode;
}

void viewer_cursor_center(ViewerCursor *cursor) {
    cursor->x_offset = ((cursor->info->view_width / 2.0) - (cursor->info->pdf_width / 2.0)) /
                                    (cursor->info->pdf_width / STEPS);
}

void viewer_cursor_set_scale(ViewerCursor *cursor, double new) {
    cursor->scale = MAX(MIN_SCALE, new);
}

void viewer_cursor_handle_offset_update(ViewerCursor *cursor) {
    int old_page;

    old_page = cursor->current_page;
    cursor->current_page =
            MIN(cursor->info->n_pages - 1,
                    MAX(0, cursor->current_page + cursor->y_offset / STEPS)
                    );

    if (cursor->y_offset < 0) {
        // If cursor->current_page just updated to 0, we still want to scroll up
        if (cursor->current_page > 0 || old_page > 0) {
            cursor->y_offset = STEPS - 1;
        } else if (cursor->current_page == 0) {
            cursor->y_offset = 0;
        }
    } else if (cursor->y_offset >= STEPS) {
        if (cursor->current_page < cursor->info->n_pages - 1 || old_page < cursor->info->n_pages - 1) {
            cursor->y_offset = 0;
        } else if (cursor->current_page == cursor->info->n_pages - 1) {
            cursor->y_offset = STEPS - 1;
        }
    }
}

void viewer_cursor_goto_page(ViewerCursor *cursor, unsigned int page) {
    cursor->current_page = page;
    cursor->y_offset = 0;
    viewer_cursor_fit_vertical(cursor);
}

void viewer_cursor_goto_poppler_dest(ViewerCursor *cursor, PopplerDest *dest) {
    // TODO: Zoom into dest, instead of just going to the page
    viewer_cursor_goto_page(cursor, dest->page_num - 1);
}

void viewer_cursor_execute_action(ViewerCursor *cursor, PopplerAction *action) {
    PopplerActionUri *action_uri;
    GError *error = NULL;
    PopplerDest *dest;

    switch (action->type) {
        case POPPLER_ACTION_URI:
            action_uri = (PopplerActionUri *)action;
            g_app_info_launch_default_for_uri(action_uri->uri, NULL, &error);
            if (error != NULL) {
                g_printerr("Poppler: Error launching URI: %s\n", error->message);
                g_error_free(error);
            }
            break;
        case POPPLER_ACTION_GOTO_DEST:
            dest = viewer_info_get_dest(cursor->info, action->goto_dest.dest);
            viewer_cursor_goto_poppler_dest(cursor, dest);
            poppler_dest_free(dest);

            break;
        default:
            g_printerr("Poppler: Unsupported link type\n");
            break;
    }
}
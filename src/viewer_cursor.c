#include <stdlib.h>

#include "viewer_cursor.h"
#include "config.h"
#include "utils.h"

ViewerCursor *viewer_cursor_new(ViewerInfo *info,
                                int current_page,
                                double x_offset,
                                double y_offset,
                                double scale,
                                bool center_mode,
                                bool dark_mode,
                                unsigned int input_number)
{
    ViewerCursor *cursor = malloc(sizeof(ViewerCursor));
    if (cursor == NULL) {
        return NULL;
    }

    viewer_cursor_init(cursor, info, current_page, x_offset, y_offset, scale, center_mode, dark_mode, input_number);

    return cursor;
}

ViewerCursor *viewer_cursor_copy(ViewerCursor *cursor)
{
    return viewer_cursor_new(cursor->info,
                             cursor->current_page,
                             cursor->x_offset,
                             cursor->y_offset,
                             cursor->scale,
                             cursor->center_mode,
                             cursor->dark_mode,
                             cursor->input_number);
}

void viewer_cursor_init(ViewerCursor *cursor,
                        ViewerInfo *info,
                        int current_page,
                        double x_offset,
                        double y_offset,
                        double scale,
                        bool center_mode,
                        bool dark_mode,
                        unsigned int input_number)
{
    cursor->info = info;
    cursor->current_page = current_page;
    cursor->x_offset = x_offset;
    cursor->y_offset = y_offset;
    cursor->scale = scale;
    cursor->center_mode = center_mode;
    cursor->dark_mode = dark_mode;
    cursor->input_number = input_number;
}

void viewer_cursor_destroy(ViewerCursor *cursor)
{
    UNUSED(cursor);
}

void viewer_cursor_fit_horizontal(ViewerCursor *cursor)
{
    cursor->scale = cursor->info->view_width / cursor->info->pdf_width;
    viewer_cursor_center(cursor);
}

void viewer_cursor_fit_vertical(ViewerCursor *cursor)
{
    cursor->scale = cursor->info->view_height / cursor->info->pdf_height;
    viewer_cursor_center(cursor);
}

void viewer_cursor_toggle_center_mode(ViewerCursor *cursor)
{
    cursor->center_mode = !cursor->center_mode;
}

void viewer_cursor_toggle_dark_mode(ViewerCursor *cursor) {
    cursor->dark_mode = !cursor->dark_mode;
}

void viewer_cursor_center(ViewerCursor *cursor)
{
    cursor->x_offset = ((cursor->info->view_width / 2.0) - (cursor->info->pdf_width / 2.0)) /
        (cursor->info->pdf_width / g_config->steps);
}

void viewer_cursor_set_scale(ViewerCursor *cursor, double new)
{
    cursor->scale = MAX(g_config->min_scale, new);
}

void viewer_cursor_handle_offset_update(ViewerCursor *cursor)
{
    const int old_page = cursor->current_page;
    cursor->current_page = CLAMP(
        cursor->current_page + cursor->y_offset / g_config->steps,
        0,
        cursor->info->n_pages - 1);

    if (cursor->y_offset < 0 && cursor->current_page < old_page) {
        cursor->y_offset = g_config->steps - 1;
    } else if (cursor->y_offset >= g_config->steps && cursor->current_page > old_page) {
        cursor->y_offset = 0;
    }
}

void viewer_cursor_goto_page(ViewerCursor *cursor, unsigned int page)
{
    cursor->current_page = page;
    cursor->y_offset = 0;
    cursor->scale = 1.0;
}

void viewer_cursor_goto_poppler_dest(ViewerCursor *cursor, PopplerDest *dest)
{
    viewer_cursor_goto_page(cursor, dest->page_num - 1);
}

void viewer_cursor_execute_action(ViewerCursor *cursor, PopplerAction *action)
{
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
        break;
    default:
        g_printerr("Poppler: Unsupported link type\n");
        break;
    }
}
#include <stdlib.h>
#include <stdbool.h>

#include "viewer.h"
#include "config.h"

Viewer *viewer_new(ViewerInfo *info, ViewerCursor *cursor, ViewerSearch *search, ViewerLinks *links)
{
    Viewer *viewer = malloc(sizeof(Viewer));
    if (viewer == NULL) {
        return NULL;
    }

    viewer_init(viewer, info, cursor, search, links);

    return viewer;
}

void viewer_init(Viewer *viewer, ViewerInfo *info, ViewerCursor *cursor, ViewerSearch *search, ViewerLinks *links)
{
    if (viewer == NULL) {
        return;
    }

    viewer->info = info;
    viewer->cursor = cursor;
    viewer->search = search;
    viewer->links = links;

    viewer->last_command = (Command){0};
    viewer->last_jump_command = (Command){0};
}

void viewer_destroy(Viewer *viewer)
{
    // FIXME: UB if member wasn't malloc'ed?
    viewer_cursor_destroy(viewer->cursor);
    free(viewer->cursor);

    viewer_search_destroy(viewer->search);
    free(viewer->search);

    viewer_links_destroy(viewer->links);
    free(viewer->links);

    viewer_info_destroy(viewer->info);
    free(viewer->info);
}

void viewer_update_current_page_size(Viewer *viewer)
{
    double max_width = 0;
    double max_height = 0;

    int from, to;
    viewer_cursor_get_visible_pages(viewer->cursor, &from, &to);
    for (int i = from; i <= to; i++) {
        PopplerPage *page = viewer_info_get_poppler_page(viewer->info, i);
        g_assert(page != NULL);
        
        double width, height;
        poppler_page_get_size(page, &width, &height);
        if (width > max_width) {
            max_width = width;
        }
        if (height > max_height) {
            max_height = height;
        }
    }

    viewer->info->pdf_width = max_width;
    viewer->info->pdf_height = max_height;
}
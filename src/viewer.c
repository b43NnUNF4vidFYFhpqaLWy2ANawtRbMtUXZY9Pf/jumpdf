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
    PopplerPage *page = viewer_info_get_poppler_page(viewer->info, viewer->cursor->current_page);

    if (page == NULL) {
        return;
    } else {
        poppler_page_get_size(page, &viewer->info->pdf_width, &viewer->info->pdf_height);
    }
}
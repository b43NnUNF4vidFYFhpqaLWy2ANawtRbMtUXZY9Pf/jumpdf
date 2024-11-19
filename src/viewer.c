#include <stdlib.h>
#include <stdbool.h>

#include "viewer.h"
#include "render.h"
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
    viewer->render_tp = NULL;
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

    if (viewer->render_tp) {
        g_thread_pool_free(viewer->render_tp, FALSE, TRUE);
    }

    g_mutex_clear(&viewer->render_mutex);
}

void viewer_init_render_tp(Viewer *viewer, GtkWidget *view)
{
    GError *error = NULL;

    viewer->render_tp = g_thread_pool_new((GFunc)render_page_async, view, 1, FALSE, &error);
    if (error != NULL) {
        g_warning("Failed to create render thread pool: %s", error->message);
        g_error_free(error);
    } else {
        g_mutex_init(&viewer->render_mutex);
    }
}
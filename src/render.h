#pragma once

#include "viewer.h"

typedef struct {
    Viewer *viewer;
    Page *page;
    unsigned int *links_drawn_sofar;
} RenderPageData;

cairo_surface_t *viewer_render(Viewer *viewer);
void render_page_async(gpointer data, gpointer user_data);
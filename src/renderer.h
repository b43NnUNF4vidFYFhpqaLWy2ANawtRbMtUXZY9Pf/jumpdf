#pragma once

#include "viewer.h"

typedef struct Renderer {
    GtkWidget *view;

    GThreadPool *render_tp;
    GMutex render_mutex;

    GPtrArray *visible_pages;
    int last_visible_pages_before, last_visible_pages_after;
    double last_scale;
    bool last_follow_links_mode;
    char *last_search_text;
} Renderer;

Renderer *renderer_new(GtkWidget *view);
void renderer_init(Renderer *renderer, GtkWidget *view);
void renderer_destroy(Renderer *renderer);

void renderer_draw(cairo_t *cr, Viewer *viewer);
void renderer_render_visible_pages(Renderer *renderer, Viewer *viewer);
void renderer_render_pages(Renderer *renderer, Viewer *viewer, unsigned int from, unsigned int to);
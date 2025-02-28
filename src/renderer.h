#pragma once

#include "viewer.h"

typedef struct Renderer {
    GtkWidget *view;

    GThreadPool *render_tp;
    GMutex render_mutex;

    GPtrArray *visible_pages;
} Renderer;

Renderer *renderer_new(GtkWidget *view);
void renderer_init(Renderer *renderer, GtkWidget *view);
void renderer_destroy(Renderer *renderer);

void renderer_draw(cairo_t *cr, Viewer *viewer);
void renderer_render_visible_pages(Renderer *renderer, Viewer *viewer);
void renderer_render_pages(Renderer *renderer, Viewer *viewer, unsigned int from, unsigned int to);
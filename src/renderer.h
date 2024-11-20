#pragma once

#include "viewer.h"

typedef struct Renderer {
    GtkWidget *view;

    GThreadPool *render_tp;
    GMutex render_mutex;
} Renderer;

Renderer *renderer_new(GtkWidget *view);
void renderer_init(Renderer *renderer, GtkWidget *view);
void renderer_destroy(Renderer *renderer);

cairo_surface_t *renderer_render(Renderer *renderer, Viewer *viewer);
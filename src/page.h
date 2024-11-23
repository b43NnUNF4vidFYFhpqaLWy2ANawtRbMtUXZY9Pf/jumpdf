#pragma once

#include <poppler.h>
#include <cairo.h>
#include <stdbool.h>

typedef enum {
    PAGE_RENDERED,
    PAGE_RENDERING,
    PAGE_NOT_RENDERED
} PageRenderStatus;

typedef struct {
    PopplerPage *poppler_page;
    PageRenderStatus render_status;
    bool is_visible;
    cairo_surface_t *surface;
} Page;

Page *page_new(PopplerPage *poppler_page);
void page_destroy(Page *page);
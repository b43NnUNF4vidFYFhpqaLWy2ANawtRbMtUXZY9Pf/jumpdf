#include "render.h"
#include "config.h"

static void viewer_update_current_page_size(Viewer *viewer);
static void viewer_zoom(Viewer *viewer, cairo_t *cr);
static void viewer_offset_translate(Viewer *viewer, cairo_t *cr);
static void viewer_render_pages(Viewer *viewer, cairo_t *cr);
static void viewer_render_page(Viewer *viewer, cairo_t *cr, PopplerPage *page, unsigned int *links_drawn_sofar);
static void viewer_highlight_search(Viewer *viewer, cairo_t *cr, PopplerPage *page);
static void viewer_draw_links(Viewer *viewer, cairo_t *cr, unsigned int from, unsigned int to);

cairo_surface_t *viewer_render(Viewer *viewer) {
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, viewer->info->view_width, viewer->info->view_height);
    cr = cairo_create(surface);

    viewer_update_current_page_size(viewer);
    viewer_zoom(viewer, cr);
    viewer_offset_translate(viewer, cr);
    viewer_render_pages(viewer, cr);

    cairo_destroy(cr);

    return surface;
}

static void viewer_update_current_page_size(Viewer *viewer)
{
    PopplerPage *page = viewer->info->pages[viewer->cursor->current_page];

    if (page == NULL) {
        return;
    } else {
        poppler_page_get_size(page, &viewer->info->pdf_width, &viewer->info->pdf_height);
    }
}

static void viewer_zoom(Viewer *viewer, cairo_t *cr) {
    double center_x = viewer->info->view_width / 2.0;
    double center_y = viewer->info->view_height / 2.0;

    cairo_translate(cr, center_x, center_y);
    cairo_scale(cr, viewer->cursor->scale, viewer->cursor->scale);
    cairo_translate(cr, -center_x, -center_y);
}

static void viewer_offset_translate(Viewer *viewer, cairo_t *cr) {
    if (viewer->cursor->center_mode) {
        viewer_cursor_center(viewer->cursor);
    }

    cairo_translate(cr,
        (viewer->cursor->x_offset / global_config->steps) * viewer->info->pdf_width,
        -(viewer->cursor->y_offset / global_config->steps) * viewer->info->pdf_height
    );
}

static void viewer_render_pages(Viewer *viewer, cairo_t *cr) {
    double visible_pages;
    unsigned int visible_pages_before, visible_pages_after;
    unsigned int links_drawn_sofar = 0;

    if (viewer->cursor->dark_mode) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
    }

    viewer_links_clear_links(viewer->links);
    viewer_render_page(viewer, cr, viewer->info->pages[viewer->cursor->current_page], &links_drawn_sofar);

    visible_pages = viewer->info->view_height / (viewer->cursor->scale * viewer->info->pdf_height);
    // TODO: Properly calculate
    visible_pages_before = ceil(visible_pages / 2);
    visible_pages_after = ceil(visible_pages / 2) + 1;

    cairo_save(cr);
    for (int i = 1; i <= visible_pages_before; i++) {
        if (viewer->cursor->current_page - i >= 0) {
            cairo_translate(cr, 0, -viewer->info->pdf_height);
            viewer_render_page(viewer, cr, viewer->info->pages[viewer->cursor->current_page - i], &links_drawn_sofar);
        }
    }
    cairo_restore(cr);
    cairo_save(cr);
    for (int i = 1; i <= visible_pages_after; i++) {
        if (viewer->cursor->current_page + i < viewer->info->n_pages) {
            cairo_translate(cr, 0, viewer->info->pdf_height);
            viewer_render_page(viewer, cr, viewer->info->pages[viewer->cursor->current_page + i], &links_drawn_sofar);
        }
    }
    cairo_restore(cr);

    if (viewer->cursor->dark_mode) {
        cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
    }
}

static void viewer_render_page(Viewer *viewer, cairo_t *cr, PopplerPage *page, unsigned int *links_drawn_sofar) {
    unsigned int links_to_draw;

    // Clear to white background (for PDFs with missing background)
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, viewer->info->pdf_width, viewer->info->pdf_height);
    cairo_fill(cr);

    poppler_page_render(page, cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_move_to(cr, 0, 0);
    cairo_rel_line_to(cr, viewer->info->pdf_width, 0);
    cairo_stroke(cr);

    viewer_highlight_search(viewer, cr, page);

    if (viewer->links->follow_links_mode) {
        links_to_draw = viewer_links_get_links(viewer->links, page);
        viewer_draw_links(viewer, cr, *links_drawn_sofar, *links_drawn_sofar + links_to_draw);
        *links_drawn_sofar += links_to_draw;
    }
}

static void viewer_highlight_search(Viewer *viewer, cairo_t *cr, PopplerPage *page) {
    PopplerRectangle *highlight_rect;
    double highlight_rect_x, highlight_rect_y, highlight_rect_width,
        highlight_rect_height;
    GList *matches;

    if (!viewer->search->search_text) {
        return;
    }

    matches = poppler_page_find_text(page, viewer->search->search_text);
    for (GList *elem = matches; elem; elem = elem->next) {
        highlight_rect = elem->data;
        highlight_rect_x = highlight_rect->x1;
        highlight_rect_y = viewer->info->pdf_height - highlight_rect->y1;
        highlight_rect_width = highlight_rect->x2 - highlight_rect->x1;
        highlight_rect_height = highlight_rect->y1 - highlight_rect->y2;

        cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.5);
        cairo_rectangle(cr, highlight_rect_x, highlight_rect_y, highlight_rect_width,
            highlight_rect_height);
        cairo_fill(cr);
    }

    g_list_free_full(matches, (GDestroyNotify)poppler_rectangle_free);
}

static void viewer_draw_links(Viewer *viewer, cairo_t *cr, unsigned int from, unsigned int to) {
    PopplerLinkMapping *link_mapping = NULL;
    char *link_text = NULL;

    for (int i = from; i < to; i++) {
        link_mapping = g_ptr_array_index(viewer->links->visible_links, i);
        link_text = g_strdup_printf("%d", i + 1);

        // Outline
        cairo_move_to(cr, link_mapping->area.x1, viewer->info->pdf_height - link_mapping->area.y1);
        cairo_text_path(cr, link_text);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_line_width(cr, 2.0);
        cairo_stroke_preserve(cr);

        // Actual text
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        cairo_fill(cr);

        g_free(link_text);
    }
}
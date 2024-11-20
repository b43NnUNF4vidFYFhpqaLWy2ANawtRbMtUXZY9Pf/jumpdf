#include "renderer.h"
#include "config.h"

typedef struct {
    Viewer *viewer;
    Page *page;
    unsigned int *links_drawn_sofar;
} RenderPageData;

static void renderer_render_pages(Renderer *renderer, Viewer *viewer);
static void render_page_async(gpointer data, gpointer user_data);
static void viewer_render_page(Viewer *viewer, cairo_t *cr, PopplerPage *page, unsigned int *links_drawn_sofar);
static cairo_surface_t* create_loading_surface(int width, int height);
static void viewer_update_current_page_size(Viewer *viewer);
static void viewer_zoom(Viewer *viewer, cairo_t *cr);
static void viewer_offset_translate(Viewer *viewer, cairo_t *cr);
static void viewer_highlight_search(Viewer *viewer, cairo_t *cr, PopplerPage *page);
static void viewer_draw_links(Viewer *viewer, cairo_t *cr, unsigned int from, unsigned int to);

Renderer *renderer_new(GtkWidget *view)
{
    Renderer *renderer = g_new0(Renderer, 1);
    renderer_init(renderer, view);

    return renderer;
}

void renderer_init(Renderer *renderer, GtkWidget *view)
{
    renderer->view = view;

    GError *error = NULL;
    renderer->render_tp = g_thread_pool_new((GFunc)render_page_async, renderer, g_get_num_processors(), FALSE, &error);
    if (error != NULL) {
        g_warning("Failed to create render thread pool: %s", error->message);
        g_error_free(error);
    } else {
        g_mutex_init(&renderer->render_mutex);
    }
}

void renderer_destroy(Renderer *renderer)
{
    g_thread_pool_free(renderer->render_tp, FALSE, TRUE);
    g_mutex_clear(&renderer->render_mutex);
}

cairo_surface_t *renderer_render(Renderer *renderer, Viewer *viewer)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    Page *page = NULL;
    double page_width, page_height;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, viewer->info->view_width, viewer->info->view_height);
    cr = cairo_create(surface);

    if (viewer->cursor->dark_mode) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
    }

    viewer_update_current_page_size(viewer);
    viewer_zoom(viewer, cr);
    viewer_offset_translate(viewer, cr);
    renderer_render_pages(renderer, viewer);

    /* TODO: Maintain a list of rendered pages */
    for (int i = 0; i < viewer->info->n_pages; i++) {
        page = viewer->info->pages[i];
        g_mutex_lock(&renderer->render_mutex);
        if (page->render_status == PAGE_RENDERED || page->render_status == PAGE_RENDERING) {
            poppler_page_get_size(page->poppler_page, &page_width, &page_height);

            g_assert(page->surface != NULL);
            cairo_set_source_surface(cr, page->surface, 0, i * page_height);
            
            /* Draw page separator */
            cairo_save(cr);
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_set_line_width(cr, 1.0);
            cairo_move_to(cr, 0, i * page_height);
            cairo_rel_line_to(cr, page_width, 0);
            cairo_stroke(cr);
            cairo_restore(cr);

            cairo_paint(cr);

            if (page->render_status == PAGE_RENDERED) {
                cairo_surface_destroy(page->surface);
                page->surface = NULL;
                page->render_status = PAGE_NOT_RENDERED;
            }
        }
        g_mutex_unlock(&renderer->render_mutex);
    }

    if (viewer->cursor->dark_mode) {
        cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
    }

    cairo_destroy(cr);

    return surface;
}

static void renderer_render_pages(Renderer *renderer, Viewer *viewer)
{
    double visible_pages;
    int visible_pages_before, visible_pages_after;
    unsigned int links_drawn_sofar = 0;
    Page *page = NULL;
    RenderPageData *data = NULL;
    GError *error = NULL;
    double width, height;

    viewer_links_clear_links(viewer->links);

    visible_pages = viewer->info->view_height / (viewer->cursor->scale * viewer->info->pdf_height);
    // TODO: Properly calculate
    visible_pages_before = MAX(0, viewer->cursor->current_page - ceil(visible_pages / 2));
    visible_pages_after = MIN(viewer->info->n_pages - 1, viewer->cursor->current_page + ceil(visible_pages / 2) + 1);

    for (int i = visible_pages_before; i <= visible_pages_after; i++) {
        page = viewer->info->pages[i];
        g_mutex_lock(&renderer->render_mutex);
        if (page->render_status == PAGE_NOT_RENDERED) {
            data = g_new0(RenderPageData, 1);
            data->viewer = viewer;
            data->page = page;
            data->links_drawn_sofar = &links_drawn_sofar;

            g_thread_pool_push(renderer->render_tp, data, &error);
            if (error != NULL) {
                g_warning("Failed to push render task to thread pool: %s", error->message);
                g_error_free(error);
            } else {
                poppler_page_get_size(page->poppler_page, &width, &height);
                page->surface = create_loading_surface(width, height);
                page->render_status = PAGE_RENDERING;
            }
        }
        g_mutex_unlock(&renderer->render_mutex);
    }
}

static void render_page_async(gpointer data, gpointer user_data)
{
    RenderPageData *render_page_data = (RenderPageData *)data;
    Viewer *viewer = render_page_data->viewer;
    Page *page = render_page_data->page;
    unsigned int *links_drawn_sofar = render_page_data->links_drawn_sofar;
    Renderer *renderer = (Renderer *)user_data;
    GtkWidget *view = renderer->view;

    double width, height;
    poppler_page_get_size(page->poppler_page, &width, &height);

    cairo_surface_t *page_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(page_surface);

    g_mutex_lock(&renderer->render_mutex);

    viewer_render_page(viewer, cr, page->poppler_page, links_drawn_sofar);

    if (page->surface) {
        cairo_surface_destroy(page->surface);
    }
    page->surface = page_surface;
    page->render_status = PAGE_RENDERED;

    g_mutex_unlock(&renderer->render_mutex);

    cairo_destroy(cr);
    g_free(render_page_data);

    gtk_widget_queue_draw(view);
}

static void viewer_render_page(Viewer *viewer, cairo_t *cr, PopplerPage *page, unsigned int *links_drawn_sofar)
{
    double width, height;
    poppler_page_get_size(page, &width, &height);

    // Clear to white background (for PDFs with missing background)
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    poppler_page_render(page, cr);

    viewer_highlight_search(viewer, cr, page);

    unsigned int links_to_draw;
    if (viewer->links->follow_links_mode) {
        links_to_draw = viewer_links_get_links(viewer->links, page);
        viewer_draw_links(viewer, cr, *links_drawn_sofar, *links_drawn_sofar + links_to_draw);
        *links_drawn_sofar += links_to_draw;
    }
}

static cairo_surface_t* create_loading_surface(int width, int height)
{
    cairo_surface_t *loading_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(loading_surface);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 32);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, "Loading...", &extents);
    double x = (width - extents.width) / 2 - extents.x_bearing;
    double y = (height - extents.height) / 2 - extents.y_bearing;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, "Loading...");

    cairo_destroy(cr);

    return loading_surface;
}

static void viewer_update_current_page_size(Viewer *viewer)
{
    PopplerPage *page = viewer_info_get_poppler_page(viewer->info, viewer->cursor->current_page);

    if (page == NULL) {
        return;
    } else {
        poppler_page_get_size(page, &viewer->info->pdf_width, &viewer->info->pdf_height);
    }
}

static void viewer_zoom(Viewer *viewer, cairo_t *cr)
{
    const double center_x = round(viewer->info->view_width / 2.0);
    const double center_y = round(viewer->info->view_height / 2.0);

    cairo_translate(cr, center_x, center_y);
    cairo_scale(cr, viewer->cursor->scale, viewer->cursor->scale);
    cairo_translate(cr, -center_x, -center_y);
}

static void viewer_offset_translate(Viewer *viewer, cairo_t *cr)
{
    if (viewer->cursor->center_mode) {
        viewer_cursor_center(viewer->cursor);
    }

    const double x_offset_translate = round((viewer->cursor->x_offset / g_config->steps) * viewer->info->pdf_width);

    /* Translation to get to current page */
    const double y_page_translate = -viewer->cursor->current_page * viewer->info->pdf_height;
    /* Translation for y offset */
    const double y_offset_translate = -(viewer->cursor->y_offset / g_config->steps) * viewer->info->pdf_height;

    const double y_translate = round(y_page_translate + y_offset_translate);

    cairo_translate(cr, x_offset_translate, y_translate);
}

static void viewer_highlight_search(Viewer *viewer, cairo_t *cr, PopplerPage *page)
{
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

static void viewer_draw_links(Viewer *viewer, cairo_t *cr, unsigned int from, unsigned int to)
{
    PopplerLinkMapping *link_mapping = NULL;
    char *link_text = NULL;

    for (unsigned int i = from; i < to; i++) {
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
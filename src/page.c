#include "page.h"

Page *page_new(PopplerPage *poppler_page)
{
    Page *page = malloc(sizeof(Page));
    if (page == NULL) {
        return NULL;
    }

    page->poppler_page = poppler_page;
    page->render_status = PAGE_NOT_RENDERED;
    page->surface = NULL;
    g_mutex_init(&page->render_mutex);

    return page;
}

void page_destroy(Page *page)
{
    if (page->poppler_page) {
        g_object_unref(page->poppler_page);
        page->poppler_page = NULL;
    }

    page->render_status = PAGE_NOT_RENDERED;

    if (page->surface) {
        cairo_surface_destroy(page->surface);
        page->surface = NULL;
    }

    g_mutex_clear(&page->render_mutex);
}
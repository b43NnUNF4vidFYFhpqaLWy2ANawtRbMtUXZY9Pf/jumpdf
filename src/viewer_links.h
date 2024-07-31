#pragma once

#include <gtk/gtk.h>
#include <poppler.h>

typedef struct ViewerLinks {
    GPtrArray *visible_links;
    bool follow_links_mode;
} ViewerLinks;

ViewerLinks *viewer_links_new(void);
void viewer_links_init(ViewerLinks *links);
void viewer_links_destroy(ViewerLinks *links);

unsigned int viewer_links_get_links(ViewerLinks *links, PopplerPage *page);
void viewer_links_clear_links(ViewerLinks *links);
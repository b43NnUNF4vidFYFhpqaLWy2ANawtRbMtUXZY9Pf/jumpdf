#pragma once

#include <gtk/gtk.h>
#include <poppler.h>

typedef struct {
    GObject parent_instance;
    gchar *title;
    int page;
    PopplerDest *dest;
    // TODO: Use some dynamic array instead, since we're just appending
    GListStore *children;
} TOCItem;

TOCItem *toc_item_new(const gchar *title, int page, PopplerDest *dest, GListStore *children);

GListModel *get_toc_tree(PopplerDocument *doc);
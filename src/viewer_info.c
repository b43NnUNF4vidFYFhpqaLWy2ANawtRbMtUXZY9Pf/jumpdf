#include "viewer_info.h"

ViewerInfo *viewer_info_new(PopplerDocument *doc) {
    ViewerInfo *info = malloc(sizeof(ViewerInfo));
    if (info == NULL) {
        return NULL;
    }

    viewer_info_init(info, doc);

    return info;
}

void viewer_info_init(ViewerInfo *info, PopplerDocument *doc) {
    info->doc = doc;
    info->n_pages = poppler_document_get_n_pages(doc);
    info->pages = malloc(sizeof(PopplerPage*) * info->n_pages);
    if (info->pages == NULL) {
        return;
    }

    for (int i = 0; i < info->n_pages; i++) {
      info->pages[i] = poppler_document_get_page(info->doc, i);

      if (!info->pages[i]) {
        g_printerr("Could not open %i'th page of document\n", i);
        g_object_unref(info->pages[i]);
      }
    }

    info->view_width = 0;
    info->view_height = 0;
    info->pdf_width = 0;
    info->pdf_height = 0;
    info->input_number = 0;
}

void viewer_info_destroy(ViewerInfo *info) {
    if (info->doc) {
        g_object_unref(info->doc);
        info->doc = NULL;
    }

    if (info->pages) {
        for (int i = 0; i < info->n_pages; i++) {
            g_object_unref(info->pages[i]);
            info->pages[i] = NULL;
        }

        free(info->pages);
        info->pages = NULL;
    }
}
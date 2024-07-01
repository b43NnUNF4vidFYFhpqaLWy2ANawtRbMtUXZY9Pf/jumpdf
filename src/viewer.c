#include <stdlib.h>
#include <stdbool.h>

#include "viewer.h"
#include "config.h"
#include "keys.h"

Viewer* viewer_new(PopplerDocument *doc) {
    Viewer* viewer = malloc(sizeof(Viewer));
    if (viewer == NULL) {
        return NULL;
    }

    viewer->doc = doc;
    viewer->n_pages = poppler_document_get_n_pages(doc);
    viewer->pages = malloc(sizeof(PopplerPage*) * viewer->n_pages);
    if (viewer->pages == NULL) {
      return NULL;
    }

    for (int i = 0; i < viewer->n_pages; i++) {
      viewer->pages[i] = poppler_document_get_page(viewer->doc, i);

      if (!viewer->pages[i]) {
        g_printerr("Could not open %i'th page of document\n", i);
        g_object_unref(viewer->pages[i]);
      }
    }

    viewer->current_page = 0;

    viewer->view_width = 0;
    viewer->view_height = 0;
    viewer->pdf_width = 0;
    viewer->pdf_height = 0;
    viewer->x_offset = 0.0;
    viewer->y_offset = 0.0;
    viewer->scale = 1.0;
    viewer->center_mode = true;

    viewer->search_text = "the";

    return viewer;
}

void viewer_destroy(Viewer* viewer) {
    if (viewer->doc) {
        g_object_unref(viewer->doc);
        viewer->doc = NULL;
    }

    if (viewer->pages) {
        for (int i = 0; i < viewer->n_pages; i++) {
            g_object_unref(viewer->pages[i]);
            viewer->pages[i] = NULL;
        }

        free(viewer->pages);
        viewer->pages = NULL;
    }
}

void viewer_fit_scale(Viewer *viewer) {
    viewer->scale = viewer->view_width / viewer->pdf_width;
    viewer_center(viewer);
}

void viewer_toggle_center_mode(Viewer *viewer) {
    viewer->center_mode = !viewer->center_mode;
}

void viewer_center(Viewer *viewer) {
    viewer->x_offset = ((viewer->view_width / 2.0) - (viewer->pdf_width / 2.0)) /
                                    (viewer->pdf_width / STEPS);
}

void viewer_set_scale(Viewer *viewer, double new) {
    viewer->scale = MAX(MIN_SCALE, new);
}

void viewer_handle_offset_update(Viewer *viewer) {
    int old_page;

    old_page = viewer->current_page;
    viewer->current_page =
            MIN(viewer->n_pages - 1,
                    MAX(0, viewer->current_page + viewer->y_offset / STEPS)
                    );

    if (viewer->y_offset < 0) {
        // If viewer->current_page just updated to 0, we still want to scroll up
        if (viewer->current_page > 0 || old_page > 0) {
            viewer->y_offset = STEPS - 1;
        } else if (viewer->current_page == 0) {
            viewer->y_offset = 0;
        }
    } else if (viewer->y_offset >= STEPS) {
        if (viewer->current_page < viewer->n_pages - 1 || old_page < viewer->n_pages - 1) {
            viewer->y_offset = 0;
        } else if (viewer->current_page == viewer->n_pages - 1) {
            viewer->y_offset = STEPS - 1;
        }
    }
}
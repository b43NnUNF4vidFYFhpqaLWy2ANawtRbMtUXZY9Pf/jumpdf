#pragma once

#include <poppler.h>

typedef struct ViewerInfo {
    PopplerDocument *doc;
    PopplerPage **pages;
    int n_pages;
    // View dimensions not known until drawn, so use draw_function to update
    int view_width, view_height;
    double pdf_width, pdf_height;
} ViewerInfo;

ViewerInfo *viewer_info_new(PopplerDocument *doc);
ViewerInfo *viewer_info_new_from_gfile(GFile *file);
void viewer_info_init(ViewerInfo *info, PopplerDocument *doc);
void viewer_info_destroy(ViewerInfo *info);

PopplerDest *viewer_info_get_dest(ViewerInfo *info, PopplerDest *dest);
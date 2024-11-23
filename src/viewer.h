#pragma once

#include "viewer_info.h"
#include "viewer_cursor.h"
#include "viewer_search.h"
#include "viewer_links.h"

typedef struct Viewer {
    ViewerInfo *info;
    ViewerCursor *cursor;
    ViewerSearch *search;
    ViewerLinks *links;
} Viewer;

Viewer *viewer_new(ViewerInfo *info, ViewerCursor *cursor, ViewerSearch *search, ViewerLinks *links);
void viewer_init(Viewer *viewer, ViewerInfo *info, ViewerCursor *cursor, ViewerSearch *search, ViewerLinks *links);
void viewer_destroy(Viewer *viewer);

void viewer_update_current_page_size(Viewer *viewer);
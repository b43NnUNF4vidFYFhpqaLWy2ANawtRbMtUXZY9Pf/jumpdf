#pragma once

#include "viewer_cursor.h"

typedef struct ViewerSearch {
    const char *search_text;
    int last_goto_page;
} ViewerSearch;

ViewerSearch *viewer_search_new(void);
void viewer_search_init(ViewerSearch *search);
void viewer_search_destroy(ViewerSearch *search);

ViewerCursor *viewer_search_get_next_search(ViewerSearch *search, ViewerCursor *current_cursor);
ViewerCursor *viewer_search_get_prev_search(ViewerSearch *search, ViewerCursor *current_cursor);
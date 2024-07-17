#pragma once

#include "viewer_cursor.h"

typedef struct ViewerMarkGroup {
    ViewerCursor *marks[9];
    unsigned int current_mark;
} ViewerMarkGroup;

ViewerMarkGroup *viewer_mark_group_new(ViewerCursor *marks[9], unsigned int current_mark);
void viewer_mark_group_init(ViewerMarkGroup *group, ViewerCursor *marks[9], unsigned int current_mark);
void viewer_mark_group_destroy(ViewerMarkGroup *group);
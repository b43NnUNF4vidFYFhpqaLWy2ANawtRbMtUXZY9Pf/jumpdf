#pragma once

#include "viewer_cursor.h"

#define NUM_MARKS 9

typedef struct ViewerMarkGroup {
    ViewerCursor *marks[NUM_MARKS];
    unsigned int current_mark;
    unsigned int previous_mark;
} ViewerMarkGroup;

ViewerMarkGroup *viewer_mark_group_new(ViewerCursor *marks[NUM_MARKS], unsigned int current_mark);
void viewer_mark_group_init(ViewerMarkGroup *group, ViewerCursor *marks[NUM_MARKS], unsigned int current_mark);
void viewer_mark_group_destroy(ViewerMarkGroup *group);
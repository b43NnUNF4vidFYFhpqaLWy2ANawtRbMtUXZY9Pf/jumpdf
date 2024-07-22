#pragma once

#include "viewer_mark_group.h"

typedef struct ViewerMarkManager {
    ViewerMarkGroup *groups[9];
    unsigned int current_group;
} ViewerMarkManager;

ViewerMarkManager *viewer_mark_manager_new(ViewerMarkGroup *groups[9], unsigned int current_group);
ViewerMarkManager *viewer_mark_manager_copy(ViewerMarkManager *manager);
void viewer_mark_manager_init(ViewerMarkManager *manager, ViewerMarkGroup *groups[9], unsigned int current_group);
void viewer_mark_manager_destroy(ViewerMarkManager *manager);

void viewer_mark_manager_set_mark(ViewerMarkManager *manager, ViewerCursor *cursor, unsigned int group_i, unsigned int mark_i);
ViewerCursor *viewer_mark_manager_get_mark(ViewerMarkManager *manager, unsigned int group_i, unsigned int mark_i);
void viewer_mark_manager_set_current_group(ViewerMarkManager *manager, unsigned int group_i);
unsigned int viewer_mark_manager_get_current_group_index(ViewerMarkManager *manager);
void viewer_mark_manager_set_current_mark(ViewerMarkManager *manager, unsigned int mark_i);
unsigned int viewer_mark_manager_get_current_mark_index(ViewerMarkManager *manager);
ViewerCursor *viewer_mark_manager_get_current_cursor(ViewerMarkManager *manager);
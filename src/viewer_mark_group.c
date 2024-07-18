#include "viewer_mark_group.h"

ViewerMarkGroup *viewer_mark_group_new(ViewerCursor *marks[9], unsigned int current_mark) {
    ViewerMarkGroup *group = malloc(sizeof(ViewerMarkGroup));
    if (group == NULL) {
        return NULL;
    }

    viewer_mark_group_init(group, marks, current_mark);

    return group;
}

void viewer_mark_group_init(ViewerMarkGroup *group, ViewerCursor *marks[9], unsigned int current_mark) {
    if (group == NULL) {
        return;
    }

    for (unsigned int i = 0; i < 9; i++) {
        group->marks[i] = marks[i];
    }

    group->current_mark = current_mark;
}

void viewer_mark_group_destroy(ViewerMarkGroup *group) {
    for (unsigned int i = 0; i < 9; i++) {
        viewer_cursor_destroy(group->marks[i]);
        free(group->marks[i]);
    }
}
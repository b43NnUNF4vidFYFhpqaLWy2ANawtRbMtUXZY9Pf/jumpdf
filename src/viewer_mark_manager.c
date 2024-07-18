#include "viewer_mark_manager.h"

ViewerMarkManager *viewer_mark_manager_new(ViewerMarkGroup *groups[9], unsigned int current_group) {
    ViewerMarkManager *manager = malloc(sizeof(ViewerMarkManager));
    if (manager == NULL) {
        return NULL;
    }

    viewer_mark_manager_init(manager, groups, current_group);

    return manager;
}

void viewer_mark_manager_init(ViewerMarkManager *manager, ViewerMarkGroup *groups[9], unsigned int current_group) {
    if (manager == NULL) {
        return;
    }

    for (unsigned int i = 0; i < 9; i++) {
        manager->groups[i] = groups[i];
    }

    manager->current_group = current_group;

}

void viewer_mark_manager_destroy(ViewerMarkManager *manager) {
    for (unsigned int i = 0; i < 9; i++) {
        viewer_mark_group_destroy(manager->groups[i]);
        free(manager->groups[i]);
    }
}

void viewer_mark_manager_set_mark(ViewerMarkManager *manager, ViewerCursor *cursor, unsigned int group_i, unsigned int mark_i) {
    ViewerMarkGroup *group = manager->groups[group_i];
    
    if (group->marks[mark_i] == NULL) {
        group->marks[mark_i] = cursor;
    } else if (group->marks[mark_i] != cursor) {
        viewer_cursor_destroy(group->marks[mark_i]);
        free(group->marks[mark_i]);
        group->marks[mark_i] = cursor;
    }
}

ViewerCursor *viewer_mark_manager_get_mark(ViewerMarkManager *manager, unsigned int group_i, unsigned int mark_i) {
    return manager->groups[group_i]->marks[mark_i];
}

void viewer_mark_manager_set_current_group(ViewerMarkManager *manager, unsigned int group_i) {
    manager->current_group = group_i;
}

unsigned int viewer_mark_manager_get_current_group_index(ViewerMarkManager *manager) {
    return manager->current_group;
}

void viewer_mark_manager_set_current_mark(ViewerMarkManager *manager, unsigned int mark_i) {
    manager->groups[manager->current_group]->current_mark = mark_i;
}

unsigned int viewer_mark_manager_get_current_mark_index(ViewerMarkManager *manager) {
    return manager->groups[manager->current_group]->current_mark;
}

ViewerCursor *viewer_mark_manager_get_current_cursor(ViewerMarkManager *manager) {
    ViewerMarkGroup *group = manager->groups[manager->current_group];
    return group->marks[group->current_mark];
}
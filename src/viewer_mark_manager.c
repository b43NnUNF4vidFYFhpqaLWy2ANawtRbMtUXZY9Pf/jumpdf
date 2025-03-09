#include "viewer_mark_manager.h"

ViewerMarkManager *viewer_mark_manager_new(ViewerMarkGroup *groups[NUM_GROUPS], unsigned int current_group)
{
    ViewerMarkManager *manager = malloc(sizeof(ViewerMarkManager));
    if (manager == NULL) {
        return NULL;
    }

    viewer_mark_manager_init(manager, groups, current_group);

    return manager;
}

ViewerMarkManager *viewer_mark_manager_copy(ViewerMarkManager *manager)
{
    return viewer_mark_manager_new(manager->groups, manager->current_group);
}

void viewer_mark_manager_init(ViewerMarkManager *manager, ViewerMarkGroup *groups[NUM_GROUPS], unsigned int current_group)
{
    if (manager == NULL) {
        return;
    }

    for (unsigned int i = 0; i < NUM_GROUPS; i++) {
        manager->groups[i] = groups[i];
    }

    manager->current_group = current_group;
}

void viewer_mark_manager_destroy(ViewerMarkManager *manager)
{
    for (unsigned int i = 0; i < NUM_GROUPS; i++) {
        viewer_mark_group_destroy(manager->groups[i]);
        free(manager->groups[i]);
    }

    free(manager->groups);
}

void viewer_mark_manager_set_mark(ViewerMarkManager *manager, ViewerCursor *cursor, unsigned int group_i, unsigned int mark_i)
{
    manager->groups[group_i]->marks[mark_i] = cursor;
}

ViewerCursor *viewer_mark_manager_get_mark(ViewerMarkManager *manager, unsigned int group_i, unsigned int mark_i)
{
    return manager->groups[group_i]->marks[mark_i];
}

void viewer_mark_manager_set_current_group(ViewerMarkManager *manager, unsigned int group_i)
{
    manager->current_group = group_i;
}

unsigned int viewer_mark_manager_get_current_group_index(ViewerMarkManager *manager)
{
    return manager->current_group;
}

void viewer_mark_manager_set_current_mark(ViewerMarkManager *manager, unsigned int mark_i)
{
    manager->groups[manager->current_group]->previous_mark = manager->groups[manager->current_group]->current_mark;
    manager->groups[manager->current_group]->current_mark = mark_i;
}

unsigned int viewer_mark_manager_get_current_mark_index(ViewerMarkManager *manager)
{
    return manager->groups[manager->current_group]->current_mark;
}

ViewerCursor *viewer_mark_manager_get_current_cursor(ViewerMarkManager *manager)
{
    ViewerMarkGroup *group = manager->groups[manager->current_group];
    return group->marks[group->current_mark];
}

void viewer_mark_manager_set_current_cursor(ViewerMarkManager *manager, ViewerCursor *cursor)
{
    ViewerMarkGroup *group = manager->groups[manager->current_group];
    group->marks[group->current_mark] = cursor;
}

void viewer_mark_manager_switch_group(ViewerMarkManager *manager, unsigned int group_i)
{
    ViewerCursor *old_cursor = viewer_mark_manager_get_current_cursor(manager);
    ViewerCursor *next_cursor = NULL;

    viewer_mark_manager_set_current_group(manager, group_i);
    next_cursor = viewer_mark_manager_get_current_cursor(manager);
    if (next_cursor == NULL) {
        viewer_mark_manager_set_current_cursor(manager, viewer_cursor_copy(old_cursor));
    }
}

void viewer_mark_manager_switch_mark(ViewerMarkManager *manager, unsigned int mark_i)
{
    unsigned int group_i = viewer_mark_manager_get_current_group_index(manager);
    ViewerCursor *stored_cursor = viewer_mark_manager_get_mark(manager, group_i, mark_i);
    ViewerCursor *current_cursor = viewer_mark_manager_get_current_cursor(manager);

    if (stored_cursor == NULL) {
        viewer_mark_manager_set_mark(manager, viewer_cursor_copy(current_cursor), group_i, mark_i);
    }

    viewer_mark_manager_set_current_mark(manager, mark_i);
}

void viewer_mark_manager_clear_mark(ViewerMarkManager *manager, unsigned int mark_i)
{
    unsigned int group_i = viewer_mark_manager_get_current_group_index(manager);
    ViewerCursor *stored_cursor = viewer_mark_manager_get_mark(manager, group_i, mark_i);
    ViewerCursor *current_cursor = viewer_mark_manager_get_current_cursor(manager);

    if (stored_cursor != current_cursor) {
        viewer_cursor_destroy(stored_cursor);
        free(stored_cursor);
        viewer_mark_manager_set_mark(manager, NULL, group_i, mark_i);
    }
}

void viewer_mark_manager_overwrite_mark(ViewerMarkManager *manager, unsigned int mark_i)
{
    unsigned int group_i = viewer_mark_manager_get_current_group_index(manager);
    ViewerCursor *stored_cursor = viewer_mark_manager_get_mark(manager, group_i, mark_i);
    ViewerCursor *current_cursor = viewer_mark_manager_get_current_cursor(manager);

    if (stored_cursor != current_cursor) {
        viewer_cursor_destroy(stored_cursor);
        free(stored_cursor);

        viewer_mark_manager_set_mark(manager, viewer_cursor_copy(current_cursor), group_i, mark_i);
        viewer_mark_manager_set_current_mark(manager, mark_i);
    }
}

void viewer_mark_manager_switch_to_previous_mark(ViewerMarkManager *manager)
{
    unsigned int group_i = viewer_mark_manager_get_current_group_index(manager);
    unsigned int previous_mark = manager->groups[group_i]->previous_mark;

    viewer_mark_manager_switch_mark(manager, previous_mark);
}
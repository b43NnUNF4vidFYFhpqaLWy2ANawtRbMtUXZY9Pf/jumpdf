#pragma once

struct Viewer;

typedef void (*CommandExecute)(struct Viewer *viewer, unsigned int repeat_count, void *data);

typedef struct {
    CommandExecute execute;
    unsigned int repeat_count;
    void *data;
} Command;

Command *command_new(CommandExecute execute, unsigned int repeat_count, void *data);
Command *command_copy(Command *command);
void command_execute(Command *command, struct Viewer *viewer);

void zoom_in(struct Viewer *viewer, unsigned int repeat_count, void *data);
void zoom_out(struct Viewer *viewer, unsigned int repeat_count, void *data);
void scroll_half_page_up(struct Viewer *viewer, unsigned int repeat_count, void *data);
void scroll_half_page_down(struct Viewer *viewer, unsigned int repeat_count, void *data);
void scroll_left(struct Viewer *viewer, unsigned int repeat_count, void *data);
void scroll_down(struct Viewer *viewer, unsigned int repeat_count, void *data);
void scroll_up(struct Viewer *viewer, unsigned int repeat_count, void *data);
void scroll_right(struct Viewer *viewer, unsigned int repeat_count, void *data);
void forward_search(struct Viewer *viewer, unsigned int repeat_count, void *data);
void backward_search(struct Viewer *viewer, unsigned int repeat_count, void *data);
void switch_to_previous_mark(struct Viewer *viewer, unsigned int repeat_count, void *data);
void switch_to_previous_group(struct Viewer *viewer, unsigned int repeat_count, void *data);
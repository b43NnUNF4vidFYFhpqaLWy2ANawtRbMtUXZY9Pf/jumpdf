#pragma once

#include <stdbool.h>

#include "viewer_info.h"

typedef struct ViewerCursor {
  ViewerInfo *info;

  int current_page;
  double x_offset, y_offset, scale;
  bool center_mode;
} ViewerCursor;

ViewerCursor *viewer_cursor_new(ViewerInfo *info);
void viewer_cursor_init(ViewerCursor *cursor, ViewerInfo *info);
void viewer_cursor_destroy(ViewerCursor *cursor);

void viewer_cursor_fit_horizontal(ViewerCursor *cursor);
void viewer_cursor_fit_vertical(ViewerCursor *cursor);
void viewer_cursor_toggle_center_mode(ViewerCursor *cursor);
void viewer_cursor_center(ViewerCursor *cursor);
void viewer_cursor_set_scale(ViewerCursor *cursor, double new_scale);
void viewer_cursor_handle_offset_update(ViewerCursor *cursor);

void viewer_cursor_goto_page(ViewerCursor *cursor, unsigned int page);
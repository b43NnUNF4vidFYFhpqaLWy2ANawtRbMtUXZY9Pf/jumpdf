#pragma once

typedef struct ViewerCursor {
  int current_page;
  double x_offset, y_offset, scale;
  bool center_mode;
} ViewerCursor;
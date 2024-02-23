#include "keys.h"
#include "config.h"
#include "win.h"

void handle_key(Window *win, guint keyval) {
  // TODO(?): Separate into functions
  switch (keyval) {
  case KEY_PLUS:
    window_set_scale(win, window_get_scale(win) + SCALE_STEP);
    break;
  case KEY_MINUS:
    window_set_scale(win, window_get_scale(win) - SCALE_STEP);
    break;
  case KEY_0:
    window_set_scale(win, 1.0);
    break;
  case KEY_c:
    window_toggle_center_mode(win);
    break;
  case KEY_s:
    window_fit_scale(win);
    break;
  case KEY_u:
    window_set_y_offset(win, window_get_y_offset(win) + STEPS / 2.0);
    break;
  case KEY_d:
    window_set_y_offset(win, window_get_y_offset(win) - STEPS / 2.0);
    break;
  case KEY_h:
    window_set_x_offset(win, window_get_x_offset(win) + 1);
    break;
  case KEY_j:
    window_set_y_offset(win, window_get_y_offset(win) - 1);
    break;
  case KEY_k:
    window_set_y_offset(win, window_get_y_offset(win) + 1);
    break;
  case KEY_l:
    window_set_x_offset(win, window_get_x_offset(win) - 1);
    break;
  }

  window_redraw(win);
}

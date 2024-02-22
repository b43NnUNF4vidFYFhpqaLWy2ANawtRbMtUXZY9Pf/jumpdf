#pragma once

#include "win.h"

typedef enum {
  KEY_PLUS = 43,
  KEY_MINUS = 45,
  KEY_0 = 48,
  KEY_c = 99,
  KEY_d = 100,
  KEY_u = 117,
  KEY_h = 104,
  KEY_j = 106,
  KEY_k = 107,
  KEY_l = 108,
  KEY_s = 115,
} Key;

void handle_key(Window *win, guint keyval);

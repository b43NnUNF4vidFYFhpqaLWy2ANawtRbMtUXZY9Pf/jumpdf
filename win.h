#pragma once

#include "app.h"
#include <gtk/gtk.h>

#define WINDOW_TYPE (window_get_type())
G_DECLARE_FINAL_TYPE(Window, window, JUMPDF, WINDOW, GtkApplicationWindow)

Window *window_new(App *app);
void window_open(Window *win, GFile *file);

void window_fit_scale(Window *win);
void window_toggle_center_mode(Window *win);
void window_center(Window *win);
void window_redraw(Window *win);

// Window is an opaque type, so getters and setters are needed to access its
// members
double window_get_x_offset(Window *win);
void window_set_x_offset(Window *win, double new);

double window_get_y_offset(Window *win);
void window_set_y_offset(Window *win, double new);

double window_get_scale(Window *win);
void window_set_scale(Window *win, double new);

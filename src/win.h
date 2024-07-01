#pragma once

#include "app.h"
#include "viewer.h"
#include <gtk/gtk.h>

#define WINDOW_TYPE (window_get_type())
G_DECLARE_FINAL_TYPE(Window, window, JUMPDF, WINDOW, GtkApplicationWindow)

Window *window_new(App *app);
void window_open(Window *win, GFile *file);

Viewer *window_get_viewer(Window *win);
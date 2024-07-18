#pragma once

#include <gtk/gtk.h>

#include "win.h"

#define APP_TYPE (app_get_type())
G_DECLARE_FINAL_TYPE(App, app, JUMPDF, APP, GtkApplication)

App *app_new(void);

void app_remove_window(App *app, Window *win);
void app_update_cursors(App *app);
void app_redraw_windows(App *app);
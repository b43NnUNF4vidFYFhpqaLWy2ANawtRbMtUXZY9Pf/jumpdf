#pragma once

#include <gtk/gtk.h>

#define APP_TYPE (app_get_type())
G_DECLARE_FINAL_TYPE(App, app, JUMPDF, APP, GtkApplication)

App *app_new(void);

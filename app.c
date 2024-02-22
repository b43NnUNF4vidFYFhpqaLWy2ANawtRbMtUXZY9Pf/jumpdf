#include <gtk/gtk.h>
#include <stdio.h>

#include "app.h"
#include "win.h"

struct _App {
  GtkApplication parent;
};

G_DEFINE_TYPE(App, app, GTK_TYPE_APPLICATION);

static void app_init(App *app) {}

static void app_activate(GApplication *app) {
  Window *win;

  win = window_new(MANYPDF_APP(app));
  gtk_window_present(GTK_WINDOW(win));
}

static void app_open(GApplication *app, GFile **files, int n_files,
                     const char *hint) {
  Window *win;

  for (int i = 0; i < n_files; i++) {
    win = window_new(MANYPDF_APP(app));
    window_open(win, files[i]);

    gtk_window_present(GTK_WINDOW(win));
  }
}

static void app_class_init(AppClass *class) {
  G_APPLICATION_CLASS(class)->activate = app_activate;
  G_APPLICATION_CLASS(class)->open = app_open;
}

App *app_new(void) {
  return g_object_new(APP_TYPE, "application-id", "org.gtk.manypdf", "flags",
                      G_APPLICATION_HANDLES_OPEN, NULL);
}

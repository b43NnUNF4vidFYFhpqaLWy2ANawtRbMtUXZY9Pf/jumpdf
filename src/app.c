#include <gtk/gtk.h>
#include <stdio.h>

#include "app.h"
#include "win.h"

struct _App {
  GtkApplication parent;

  GHashTable *file_viewer_mark_manager_map;
  GPtrArray *windows;
};

G_DEFINE_TYPE(App, app, GTK_TYPE_APPLICATION);

static void app_init(App *app) {
  app->file_viewer_mark_manager_map = g_hash_table_new(g_str_hash, g_str_equal);
  app->windows = g_ptr_array_new();
}

static void app_finalize(GObject *object) {
  App *app = JUMPDF_APP(object);

  g_hash_table_destroy(app->file_viewer_mark_manager_map);
  g_ptr_array_free(app->windows, TRUE);

  G_OBJECT_CLASS(app_parent_class)->finalize(object);
}

static void app_activate(GApplication *app) {
  g_printerr("Jumpdf: Files not specified\n");
}

static void app_open(GApplication *app, GFile **files, int n_files,
                     const char *hint) {
  char *uri;
  ViewerMarkGroup **groups;
  ViewerCursor **default_cursors;
  ViewerMarkManager *mark_manager;
  Window *win;
  GPtrArray *new_windows = g_ptr_array_sized_new(n_files);

  for (int i = 0; i < n_files; i++) {
    uri = g_file_get_uri(files[i]);
    if (!g_hash_table_contains(JUMPDF_APP(app)->file_viewer_mark_manager_map, uri)) {
      // TODO: Fetch ViewerMarkManager of URI from DB (temporary)
      groups = malloc(9 * sizeof(ViewerMarkGroup *));

      for (unsigned int i = 0; i < 9; i++) {
        default_cursors = malloc(9 * sizeof(ViewerCursor *));
        for (unsigned int j = 0; j < 9; j++) {
          default_cursors[j] = NULL;
        }
        groups[i] = viewer_mark_group_new(default_cursors, 0);
      }

      g_hash_table_insert(JUMPDF_APP(app)->file_viewer_mark_manager_map,
                          uri, groups);
    } else {
      groups = g_hash_table_lookup(JUMPDF_APP(app)->file_viewer_mark_manager_map, uri);
    }
    mark_manager = viewer_mark_manager_new(groups, 0);

    win = window_new(JUMPDF_APP(app));
    g_ptr_array_add(new_windows, win);

    window_open(win, files[i], mark_manager);
    gtk_window_present(GTK_WINDOW(win));
  }

  g_ptr_array_extend_and_steal(JUMPDF_APP(app)->windows, new_windows);
}

static void app_class_init(AppClass *class) {
  G_OBJECT_CLASS(class)->finalize = app_finalize;
  G_APPLICATION_CLASS(class)->activate = app_activate;
  G_APPLICATION_CLASS(class)->open = app_open;
}

App *app_new(void) {
  return g_object_new(APP_TYPE, "application-id", "org.gtk.jumpdf", "flags",
                      G_APPLICATION_HANDLES_OPEN, NULL);
}

void app_redraw_windows(App *app) {
  g_ptr_array_foreach(app->windows, (GFunc)window_redraw, NULL);
}
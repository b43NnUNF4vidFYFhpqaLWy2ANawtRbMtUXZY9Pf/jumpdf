#include <gtk/gtk.h>
#include <stdio.h>

#include "app.h"
#include "config.h"
#include "database.h"

static void database_update_mark_manager_cb(gpointer uri_ptr, gpointer manager_ptr, gpointer user_data);

static void on_file_dialog_response(GObject *source_object, GAsyncResult *res, gpointer user_data);

struct _App {
  GtkApplication parent;

  /*
  * Key: URI
  * Value: ViewerMarkManager *
  * NOTE: a URI can have multiple mark managers with the same groups,
  * but with a different current_group. The value here is just a default
  */
  GHashTable *uri_mark_manager_map;
  GPtrArray *windows;
};

G_DEFINE_TYPE(App, app, GTK_TYPE_APPLICATION);

int app_run(int argc, char *argv[]) {
  App *app = app_new();
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}

static void app_init(App *app) {
  config_load(&global_config);
  database_create_tables(database_get_instance());
  app->uri_mark_manager_map = g_hash_table_new(g_str_hash, g_str_equal);
  app->windows = g_ptr_array_new();
}

static void app_finalize(GObject *object) {
  App *app = JUMPDF_APP(object);

  app_update_database_mark_managers(app);
  g_hash_table_destroy(app->uri_mark_manager_map);

  g_ptr_array_free(app->windows, TRUE);

  database_close(database_get_instance());
  g_free(global_config.db_filename);

  G_OBJECT_CLASS(app_parent_class)->finalize(object);
}

static void app_activate(GApplication *app) {
  app_open_file_chooser(JUMPDF_APP(app));
}

static void app_open(GApplication *app, GFile **files, int n_files,
                     const char *hint) {
  ViewerMarkManager *mark_manager;
  Window *win;
  GPtrArray *new_windows = g_ptr_array_sized_new(n_files);

  for (int i = 0; i < n_files; i++) {
    win = window_new(JUMPDF_APP(app));
    g_ptr_array_add(new_windows, win);

    mark_manager = app_get_mark_manager(JUMPDF_APP(app), files[i]);
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

ViewerMarkManager *app_get_mark_manager(App *app, GFile *file) {
  Database *db = database_get_instance();
  char *uri = g_file_get_uri(file);
  ViewerInfo *info = viewer_info_new_from_gfile(file);
  ViewerMarkGroup **groups;
  ViewerCursor **default_cursors;
  ViewerCursor *default_cursor;
  ViewerCursor *cursor;
  ViewerMarkManager *mark_manager;
  ViewerMarkManager *mark_manager_db;

  if (!g_hash_table_contains(JUMPDF_APP(app)->uri_mark_manager_map, uri)) {
    app_update_database_mark_managers(JUMPDF_APP(app));
    mark_manager = database_get_mark_manager(db, uri);
    if (mark_manager == NULL) {
      groups = malloc(NUM_GROUPS * sizeof(ViewerMarkGroup *));

      for (unsigned int i = 0; i < NUM_GROUPS; i++) {
        default_cursors = malloc(NUM_MARKS * sizeof(ViewerCursor *));
        for (unsigned int j = 0; j < NUM_MARKS; j++) {
          default_cursors[j] = NULL;
        }
        groups[i] = viewer_mark_group_new(default_cursors, 0);
      }

      mark_manager = viewer_mark_manager_new(groups, 0);
      database_insert_mark_manager(db, uri, mark_manager);
    }

    g_hash_table_insert(JUMPDF_APP(app)->uri_mark_manager_map,
                        uri, mark_manager);
  } else {
    mark_manager_db = g_hash_table_lookup(JUMPDF_APP(app)->uri_mark_manager_map, uri);
    mark_manager = viewer_mark_manager_copy(mark_manager_db);
  }

  for (unsigned int j = 0; j < NUM_GROUPS; j++)  {
    for (unsigned int k = 0; k < NUM_MARKS; k++) {
      cursor = viewer_mark_manager_get_mark(mark_manager, j, k);
      if (cursor != NULL) {
        cursor->info = info;
      }
    }
  }

  if (viewer_mark_manager_get_current_cursor(mark_manager) == NULL) {
    default_cursor = viewer_cursor_new(info, 0, 0.0, 0.0, 1.0, TRUE, 0);
    viewer_mark_manager_set_mark(mark_manager, viewer_cursor_copy(default_cursor),
      viewer_mark_manager_get_current_group_index(mark_manager),
      viewer_mark_manager_get_current_mark_index(mark_manager));

    viewer_cursor_destroy(default_cursor);
    free(default_cursor);
  }

  return mark_manager;
}

void app_remove_window(App *app, Window *win) {
  g_ptr_array_remove_fast(app->windows, win);
}

void app_update_cursors(App *app) {
  g_ptr_array_foreach(app->windows, (GFunc)window_update_cursor, NULL);
}

void app_redraw_windows(App *app) {
  g_ptr_array_foreach(app->windows, (GFunc)window_redraw, NULL);
}

void app_update_database_mark_managers(App *app) {
  g_hash_table_foreach(app->uri_mark_manager_map,
                       (GHFunc)database_update_mark_manager_cb, 
                       NULL);
}

void app_open_file_chooser(App *app) {
  GtkFileDialog *file_dialog = gtk_file_dialog_new();
  g_application_hold(G_APPLICATION(app));
  gtk_file_dialog_open_multiple(file_dialog, NULL, NULL, (GAsyncReadyCallback)on_file_dialog_response, app);
}

static void database_update_mark_manager_cb(gpointer uri_ptr, gpointer manager_ptr, gpointer user_data) {
  const char *uri = uri_ptr;
  ViewerMarkManager *manager = manager_ptr;
  Database *db = database_get_instance();

  database_update_mark_manager(db, uri, manager);
}

static void on_file_dialog_response(GObject *source_object, GAsyncResult *res, gpointer user_data) {
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
  GError *error = NULL;
  GListModel *files_model = gtk_file_dialog_open_multiple_finish(dialog, res, &error);
  GApplication *app = G_APPLICATION(user_data);
  int n_files = 0;
  GFile **file_array = NULL;

  if (error != NULL) {
    g_printerr("Error opening files: %s\n", error->message);
    g_error_free(error);
  } else if (files_model != NULL) {
    n_files = g_list_model_get_n_items(files_model);
    file_array = g_new(GFile *, n_files);
    for (int i = 0; i < n_files; i++) {
      file_array[i] = G_FILE(g_list_model_get_item(files_model, i));
    }

    app_open(app, file_array, n_files, NULL);

    for (int i = 0; i < n_files; i++) {
      g_object_unref(file_array[i]);
    }
    g_free(file_array);

    g_object_unref(files_model);
  }

  g_object_unref(dialog);
  g_application_release(app);
}
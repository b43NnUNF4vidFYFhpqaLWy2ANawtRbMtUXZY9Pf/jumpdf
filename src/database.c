#include <stddef.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "database.h"
#include "config.h"

static Database *database_new();
static void database_init(Database *db);
static void ensure_path_exists(const char *path);
static gchar *get_db_file();

struct Database {
    sqlite3 *db;
};

Database *database_get_instance() {
    static Database *instance = NULL;

    if (instance == NULL) {
        instance = database_new();
    }

    return instance;
}

void database_close() {
    Database *db = database_get_instance();
    int rc;

    if (db == NULL) {
        return;
    }

    rc = sqlite3_close(db->db);
    if (rc != SQLITE_OK) {
        g_printerr("sqlite3: %s\n", sqlite3_errmsg(db->db));
    }
}

static Database *database_new() {
    Database *db = malloc(sizeof(Database));
    if (db == NULL) {
        return NULL;
    }

    database_init(db);

    return db;
}

static void database_init(Database *db) {
    gchar *db_file = get_db_file();
    int rc;

    if (db == NULL) {
        return;
    }

    ensure_path_exists(db_file);

    rc = sqlite3_open(db_file, &(db->db));
    if (rc != SQLITE_OK) {
        g_printerr("sqlite3: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        return;
    }

    g_free(db_file);
}

static void ensure_path_exists(const char *path) {
    gchar *dir = g_path_get_dirname(path);
    gint rc = g_mkdir_with_parents(dir, 0700);
    int saved_errno = errno;

    if (rc == -1) {
        g_printerr("GLib: %s\n", g_strerror(saved_errno));
    }

    g_free(dir);
}

static gchar *get_db_file() {
    return g_build_filename(g_get_home_dir(), DB_FILENAME_REL_HOME, NULL);
}
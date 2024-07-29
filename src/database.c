#include <stddef.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "database.h"
#include "config.h"

// TODO: A lot of repetition in these functions

static Database *database_new();
static void database_init(Database *db);
static void database_printerr_stmt(Database *db, sqlite3_stmt *stmt);
static void database_printerr_sql(Database *db, char *sql);

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

void database_close(Database *db) {
    int rc;

    if (db == NULL) {
        return;
    }

    rc = sqlite3_close(db->db);
    if (rc != SQLITE_OK) {
        g_printerr("database_close: %s\n", sqlite3_errmsg(db->db));
    }
}

void database_create_tables(Database *db) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS cursor ("
        "   id INTEGER PRIMARY KEY AUTOINCREMENT," 
        "   current_page INTEGER NOT NULL,"
        "   x_offset REAL NOT NULL,"
        "   y_offset REAL NOT NULL,"
        "   scale REAL NOT NULL,"
        "   center_mode BOOLEAN NOT NULL,"
        "   input_number INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS cursor_group ("
        "   id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   current_mark INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS mark_manager ("
        "   uri TEXT PRIMARY KEY,"
        "   current_group INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS group_cursor ("
        "   group_id INTEGER NOT NULL,"
        "   cursor_id INTEGER NOT NULL,"
        "   cursor_index INTEGER NOT NULL,"
        "   PRIMARY KEY(group_id, cursor_index),"
        "   FOREIGN KEY(group_id) REFERENCES cursor_group(id),"
        "   FOREIGN KEY(cursor_id) REFERENCES cursor(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS mark_manager_group ("
        "   id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   mark_manager_uri INTEGER NOT NULL,"
        "   group_id INTEGER NOT NULL,"
        "   group_index INTEGER NOT NULL,"
        "   FOREIGN KEY(mark_manager_uri) REFERENCES mark_manager(uri),"
        "   FOREIGN KEY(group_id) REFERENCES cursor_group(id)"
        ");"
        ;
    char *errmsg = NULL;
    int rc = sqlite3_exec(db->db, sql, NULL, NULL, &errmsg);

    if (rc != SQLITE_OK) {
        g_printerr("database_create_tables: %s\n", errmsg);
        sqlite3_free(errmsg);
    }
}

sqlite3_int64 database_insert_cursor(Database *db, ViewerCursor *cursor) {
    const char *sql = "INSERT INTO cursor (current_page, x_offset, y_offset, scale, center_mode, input_number) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc;
    int cursor_id = -1;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_insert_cursor: %s\n", sqlite3_errmsg(db->db));
        return -1;
    } 

    sqlite3_bind_int(stmt, 1, cursor->current_page);
    sqlite3_bind_double(stmt, 2, cursor->x_offset);
    sqlite3_bind_double(stmt, 3, cursor->y_offset);
    sqlite3_bind_double(stmt, 4, cursor->scale);
    sqlite3_bind_int(stmt, 5, cursor->center_mode);
    sqlite3_bind_int(stmt, 6, cursor->input_number);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    } else {
        cursor_id = sqlite3_last_insert_rowid(db->db);
    }

    sqlite3_finalize(stmt);

    return cursor_id;
}

void database_update_cursor(Database *db, int id, ViewerCursor *cursor) {
    const char *sql =
        "UPDATE cursor "
        "SET current_page = ?, x_offset = ?, y_offset = ?, scale = ?, center_mode = ?, input_number = ? "
        "WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_update_cursor: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    sqlite3_bind_int(stmt, 1, cursor->current_page);
    sqlite3_bind_double(stmt, 2, cursor->x_offset);
    sqlite3_bind_double(stmt, 3, cursor->y_offset);
    sqlite3_bind_double(stmt, 4, cursor->scale);
    sqlite3_bind_int(stmt, 5, cursor->center_mode);
    sqlite3_bind_int(stmt, 6, cursor->input_number);
    sqlite3_bind_int(stmt, 7, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    }

    sqlite3_finalize(stmt);
}

ViewerCursor *database_get_cursor(Database *db, int id) {
    const char *sql = "SELECT current_page, x_offset, y_offset, scale, center_mode, input_number FROM cursor WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc;
    int current_page;
    double x_offset, y_offset, scale;
    int center_mode;
    int input_number;
    ViewerCursor *cursor = NULL;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_get_cursor: %s\n", sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        current_page = sqlite3_column_int(stmt, 0);
        x_offset = sqlite3_column_double(stmt, 1);
        y_offset = sqlite3_column_double(stmt, 2);
        scale = sqlite3_column_double(stmt, 3);
        center_mode = sqlite3_column_int(stmt, 4);
        input_number = sqlite3_column_int(stmt, 5);

        cursor = viewer_cursor_new(NULL, current_page, x_offset, y_offset, scale, center_mode, input_number);
    } else {
        database_printerr_stmt(db, stmt);
    }

    sqlite3_finalize(stmt);

    return cursor;
}

sqlite3_int64 database_insert_group(Database *db, ViewerMarkGroup *group) {
    const char *sql = "INSERT INTO cursor_group (current_mark) VALUES (?);";
    sqlite3_stmt *stmt;
    int rc;
    sqlite3_int64 group_id = -1;
    sqlite3_int64 cursor_id = -1;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_insert_group: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, group->current_mark);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    } else {
        group_id = sqlite3_last_insert_rowid(db->db);
    }

    sqlite3_finalize(stmt);

    for (unsigned int i = 0; i < 9; i++) {
        if (group->marks[i] != NULL) {
            cursor_id = database_insert_cursor(db, group->marks[i]);
            database_insert_group_cursor(db, group_id, cursor_id, i);
        }
    }

    return group_id;
}

sqlite3_int64 database_insert_group_cursor(Database *db, int group_id, int cursor_id, int cursor_index) {
    const char *sql = 
        "INSERT INTO "
        "group_cursor (group_id, cursor_id, cursor_index) "
        "VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc;
    sqlite3_int64 group_cursor_id = -1;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_insert_group_cursor: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, cursor_id);
    sqlite3_bind_int(stmt, 3, cursor_index);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    } else {
        group_cursor_id = sqlite3_last_insert_rowid(db->db);
    }

    sqlite3_finalize(stmt);

    return group_cursor_id;
}

void database_update_group(Database *db, int id, ViewerMarkGroup *group) {
    const char *sql =
        "UPDATE cursor_group "
        "SET current_mark = ? "
        "WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc;
    ViewerCursor **cursors_db;
    int new_cursor_id = -1;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_update_group: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    sqlite3_bind_int(stmt, 1, group->current_mark);
    sqlite3_bind_int(stmt, 2, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    }

    sqlite3_finalize(stmt);

    cursors_db = database_get_group_cursors(db, id);
    for (unsigned int i = 0; i < 9; i++) {
        if (cursors_db[i] == NULL && group->marks[i] != NULL) {
            new_cursor_id = database_insert_cursor(db, group->marks[i]);
            database_insert_group_cursor(db, id, new_cursor_id, i);
        } else if (cursors_db[i] != NULL) {
            database_update_cursor_in_group(db, id, i, group->marks[i]);
        }
    }

    for (unsigned int i = 0; i < 9; i++) {
        if (cursors_db[i] != NULL) {
            viewer_cursor_destroy(cursors_db[i]);
            free(cursors_db[i]);
        }
    }
}

void database_update_cursor_in_group(Database *db, int group_id, int cursor_index, ViewerCursor *cursor) {
    const char *sql =
        "SELECT cursor_id "
        "FROM group_cursor "
        "WHERE group_id = ? AND cursor_index = ?;";
    sqlite3_stmt *stmt;
    int rc;
    int cursor_id;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_update_cursor_in_group: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    sqlite3_bind_int(stmt, 1, group_id);
    sqlite3_bind_int(stmt, 2, cursor_index);

    while (TRUE) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            cursor_id = sqlite3_column_int(stmt, 0);
            database_update_cursor(db, cursor_id, cursor);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            database_printerr_stmt(db, stmt);
        }
    }

    sqlite3_finalize(stmt);
}

ViewerMarkGroup *database_get_group(Database *db, int id) {
    const char *sql = "SELECT current_mark FROM cursor_group WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc;
    ViewerCursor **cursors;
    int current_mark;
    ViewerMarkGroup *group = NULL;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_get_group: %s\n", sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        cursors = database_get_group_cursors(db, id);
        current_mark = sqlite3_column_int(stmt, 0);
        group = viewer_mark_group_new(cursors, current_mark);
    } else {
        database_printerr_stmt(db, stmt);
    }

    sqlite3_finalize(stmt);

    return group;
}

ViewerCursor **database_get_group_cursors(Database *db, int id) {
    const char *sql = 
        "SELECT cursor_id, cursor_index "
        "FROM group_cursor "
        "WHERE group_id = ?;";
    sqlite3_stmt *stmt;
    int rc;
    int cursor_id;
    int cursor_index;
    ViewerCursor **cursors = malloc(9 * sizeof(ViewerCursor *));

    for (unsigned int i = 0; i < 9; i++) {
        cursors[i] = NULL;
    }

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_get_group_cursors: %s\n", sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);

    while (TRUE) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            cursor_id = sqlite3_column_int(stmt, 0);
            cursor_index = sqlite3_column_int(stmt, 1);
            cursors[cursor_index] = database_get_cursor(db, cursor_id);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            database_printerr_stmt(db, stmt);
        }
    }

    sqlite3_finalize(stmt);

    return cursors;
}

void database_insert_mark_manager(Database *db, const char *uri, ViewerMarkManager *manager) {
    const char *sql = "INSERT INTO mark_manager (uri, current_group) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc;
    sqlite3_int64 group_id = -1;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_insert_mark_manager: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, manager->current_group);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    }

    sqlite3_finalize(stmt);

    for (unsigned int i = 0; i < 9; i++) {
        if (manager->groups[i] != NULL) {
            group_id = database_insert_group(db, manager->groups[i]);
            database_insert_mark_manager_group(db, uri, group_id, i);
        }
    }
}

sqlite3_int64 database_insert_mark_manager_group(Database *db, const char *uri, int group_id, int group_index) {
    const char *sql =
        "INSERT INTO "
        "mark_manager_group (mark_manager_uri, group_id, group_index) "
        "VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc;
    sqlite3_int64 mark_manager_group_id = -1;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_insert_mark_manager_group: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, group_id);
    sqlite3_bind_int(stmt, 3, group_index);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    } else {
        mark_manager_group_id = sqlite3_last_insert_rowid(db->db);
    }

    sqlite3_finalize(stmt);

    return mark_manager_group_id;
}

void database_update_mark_manager(Database *db, const char *uri, ViewerMarkManager *manager) {
    const char *sql =
        "UPDATE mark_manager "
        "SET current_group = ? "
        "WHERE uri = ?;";
    sqlite3_stmt *stmt;
    int rc;
    ViewerMarkGroup **groups_db;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_update_mark_manager: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    sqlite3_bind_int(stmt, 1, manager->current_group);
    sqlite3_bind_text(stmt, 2, uri, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        database_printerr_stmt(db, stmt);
    }

    sqlite3_finalize(stmt);

    database_update_groups_in_mark_manager(db, uri, manager->groups);
    groups_db = database_get_mark_manager_groups(db, uri);
    for (unsigned int i = 0; i < 9; i++) {
        if (groups_db[i] == NULL && manager->groups[i] != NULL) {
            database_insert_group(db, manager->groups[i]);
        }
    }

    for (unsigned int i = 0; i < 9; i++) {
        if (groups_db[i] != NULL) {
            viewer_mark_group_destroy(groups_db[i]);
            free(groups_db[i]);
        }
    }
}

void database_update_groups_in_mark_manager(Database *db, const char *uri, ViewerMarkGroup **groups) {
    const char *sql =
        "SELECT group_id, group_index "
        "FROM mark_manager_group "
        "WHERE mark_manager_uri = ?;";
    sqlite3_stmt *stmt;
    int rc;
    int group_id;
    int group_index;

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_update_groups_in_mark_manager: %s\n", sqlite3_errmsg(db->db));
        return;
    }

    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_STATIC);

    while (TRUE) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            group_id = sqlite3_column_int(stmt, 0);
            group_index = sqlite3_column_int(stmt, 1);
            database_update_group(db, group_id, groups[group_index]);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            database_printerr_stmt(db, stmt);
        }
    }

    sqlite3_finalize(stmt);
}

ViewerMarkManager *database_get_mark_manager(Database *db, const char *uri) {
    const char *sql = "SELECT current_group FROM mark_manager WHERE uri = ?;";
    sqlite3_stmt *stmt;
    int rc;
    int current_group;
    ViewerMarkGroup **groups = malloc(9 * sizeof(ViewerMarkGroup *));
    ViewerMarkManager *manager = NULL;

    for (unsigned int i = 0; i < 9; i++) {
        groups[i] = NULL;
    }

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_get_mark_manager: %s\n", sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        current_group = sqlite3_column_int(stmt, 0);
        groups = database_get_mark_manager_groups(db, uri);
        manager = viewer_mark_manager_new(groups, current_group);
    } else if (rc == SQLITE_DONE) {
        // No mark manager found
    } else {
        database_printerr_stmt(db, stmt);
    }

    sqlite3_finalize(stmt);

    return manager;
}

ViewerMarkGroup **database_get_mark_manager_groups(Database *db, const char *uri) {
    const char *sql = 
        "SELECT group_id, group_index "
        "FROM mark_manager_group "
        "WHERE mark_manager_uri = ?;";
    sqlite3_stmt *stmt;
    int rc;
    int group_id;
    int group_index;
    ViewerMarkGroup **groups = malloc(9 * sizeof(ViewerMarkGroup *));

    for (unsigned int i = 0; i < 9; i++) {
        groups[i] = NULL;
    }

    rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        g_printerr("database_get_mark_manager_groups: %s\n", sqlite3_errmsg(db->db));
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_STATIC);

    while (TRUE) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            group_id = sqlite3_column_int(stmt, 0);
            group_index = sqlite3_column_int(stmt, 1);
            groups[group_index] = database_get_group(db, group_id);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            database_printerr_stmt(db, stmt);
        }
    }

    sqlite3_finalize(stmt);

    return groups;
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
        g_printerr("database_init: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        return;
    }

    g_free(db_file);
}

static void database_printerr_stmt(Database *db, sqlite3_stmt *stmt) {
    char *query = sqlite3_expanded_sql(stmt);
    if (query == NULL) {
        g_printerr("database_printerr: sqlite3_expanded_sql failed\n");
    } else {
        database_printerr_sql(db, query);
    }

}
static void database_printerr_sql(Database *db, char *sql) {
    g_print("\"%s\": %s\n", sql, sqlite3_errmsg(db->db));
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
    return g_build_filename(g_get_home_dir(), global_config.db_filename, NULL);
}
#pragma once

#include <glib.h>

#include "statusline.h"

typedef struct Config {
    gchar *db_filename;
    gchar *file_chooser_initial_folder_path;
    int steps;
    double min_scale;
    double scale_step;

    gchar *statusline_separator;
    GArray *statusline_left;
    GArray *statusline_middle;
    GArray *statusline_right;
} Config;

extern Config *global_config;

Config *config_new(void);
void config_init(Config *config);
void config_destroy(Config *config);

void config_set_db_filename(Config *config, gchar *db_filename);
void config_set_file_chooser_initial_folder_path(Config *config, gchar *file_chooser_initial_folder_path);
void config_set_steps(Config *config, int steps);
void config_set_min_scale(Config *config, double min_scale);
void config_set_scale_step(Config *config, double scale_step);
void config_set_statusline_separator(Config *config, gchar *statusline_separator);

void config_load(Config *config);
void config_load_default(Config *config);
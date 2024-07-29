#pragma once

#include <gtk/gtk.h>

typedef struct Config {
    gchar *db_filename;
    int steps;
    double min_scale;
    double scale_step;
} Config;

extern Config global_config;

void config_set_db_filename(Config *config, gchar *db_filename);
void config_set_steps(Config *config, int steps);
void config_set_min_scale(Config *config, double min_scale);
void config_set_scale_step(Config *config, double scale_step);
void config_load(Config *config);
void config_load_default(Config *config);
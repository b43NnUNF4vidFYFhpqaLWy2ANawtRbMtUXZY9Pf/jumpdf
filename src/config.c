#include <stdlib.h>
#include <toml.h>

#include "config.h"

#define DEFAULT_DB_FILENAME "~/.local/share/jumpdf/jumpdf.db"
#define DEFAULT_FILE_CHOOSER_INITIAL_FOLDER_PATH "~/Documents"
#define DEFAULT_STEPS 15 // Number of steps in a page.
#define DEFAULT_MIN_SCALE 0.3 // To prevent divide by zero
#define DEFAULT_SCALE_STEP 0.1 // How much to scale the PDF on each event

static void config_parse(Config *config, FILE *fp);

Config global_config = {
    .db_filename = NULL,
    .file_chooser_initial_folder_path = NULL,
    .steps = -1,
    .min_scale = -1.0,
    .scale_step = -1.0
};

void config_set_db_filename(Config *config, gchar *db_filename) {
    config->db_filename = db_filename;
}

void config_set_file_chooser_initial_folder_path(Config *config, gchar *file_chooser_initial_folder_path) {
    config->file_chooser_initial_folder_path = file_chooser_initial_folder_path;
}

void config_set_steps(Config *config, int steps) {
    if (steps <= 0) {
        g_printerr("\"steps\" must be greater than 0. Using default value.\n");
        config->steps = DEFAULT_STEPS;
    } else {
        config->steps = steps;
    }
}

void config_set_min_scale(Config *config, double min_scale) {
    if (min_scale <= 0.0) {
        g_printerr("\"min_scale\" must be greater than 0.0. Using default value.\n");
        config->min_scale = DEFAULT_MIN_SCALE;
    } else {
        config->min_scale = min_scale;
    }
}

void config_set_scale_step(Config *config, double scale_step) {
    if (scale_step <= 0.0) {
        g_printerr("\"scale_step\" must be greater than 0.0. Using default value.\n");
        config->scale_step = DEFAULT_SCALE_STEP;
    } else {
        config->scale_step = scale_step;
    }
}

void config_load(Config *config) {
    FILE *fp;
    gchar *config_file_path;

    config_file_path = g_build_filename(g_get_home_dir(), ".config", "jumpdf", "config.toml", NULL);
    fp = fopen(config_file_path, "r");
    g_free(config_file_path);

    if (fp) {
        config_parse(config, fp);
        fclose(fp);
    } else {
        config_load_default(config);
    }
}

void config_load_default(Config *config) {
    config_set_db_filename(config, g_strdup(DEFAULT_DB_FILENAME));
    config_set_file_chooser_initial_folder_path(config, g_strdup(DEFAULT_FILE_CHOOSER_INITIAL_FOLDER_PATH));
    config_set_steps(config, DEFAULT_STEPS);
    config_set_min_scale(config, DEFAULT_MIN_SCALE);
    config_set_scale_step(config, DEFAULT_SCALE_STEP);
}

static void config_parse(Config *config, FILE *fp) {
    toml_table_t *conf;
    char errbuf[256];
    toml_table_t *settings;
    toml_datum_t datum;

    conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    if (conf == 0) {
        g_printerr("Error parsing config file: %s\n\tUsing default values.\n", errbuf);
        config_load_default(config);
        return;
    }

    settings = toml_table_in(conf, "Settings");
    if (settings) {
        datum = toml_string_in(settings, "db_filename");
        if (datum.ok) {
            config_set_db_filename(config, datum.u.s);
        } else {
            g_printerr("Error parsing \"db_filename\". Using default value.\n");
            config_set_db_filename(config, g_strdup(DEFAULT_DB_FILENAME));
        }

        datum = toml_string_in(settings, "file_chooser_initial_folder_path");
        if (datum.ok) {
            config_set_file_chooser_initial_folder_path(config, datum.u.s);
        } else {
            g_printerr("Error parsing \"file_chooser_initial_folder_path\". Using default value.\n");
            config_set_file_chooser_initial_folder_path(config, g_strdup(DEFAULT_FILE_CHOOSER_INITIAL_FOLDER_PATH));
        }

        datum = toml_int_in(settings, "steps");
        if (datum.ok) {
            config_set_steps(config, datum.u.i);
        } else {
            g_printerr("Error parsing \"steps\". Using default value.\n");
            config_set_steps(config, DEFAULT_STEPS);
        }

        datum = toml_double_in(settings, "min_scale");
        if (datum.ok) {
            config_set_min_scale(config, datum.u.d);
        } else {
            g_printerr("Error parsing \"min_scale\". Using default value.\n");
            config_set_min_scale(config, DEFAULT_MIN_SCALE);
        }

        datum = toml_double_in(settings, "scale_step");
        if (datum.ok) {
            config_set_scale_step(config, datum.u.d);
        } else {
            g_printerr("Error parsing \"scale_step\". Using default value.\n");
            config_set_scale_step(config, DEFAULT_SCALE_STEP);
        }
    } else {
        g_printerr("Error parsing config file: No \"Settings\" table found. Using default values.\n");
        config_load_default(config);
    }

    toml_free(conf);
}
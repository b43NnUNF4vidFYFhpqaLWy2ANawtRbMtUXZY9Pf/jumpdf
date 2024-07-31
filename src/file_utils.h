#pragma once

#include <glib.h>

void ensure_path_exists(const char *path);
gchar *expand_path(char *path);
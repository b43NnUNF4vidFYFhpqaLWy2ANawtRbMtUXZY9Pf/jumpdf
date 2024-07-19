#pragma once

#include "sqlite3.h"

// Singleton unnecessary?
typedef struct Database Database;

Database *database_get_instance();
void database_close();
#ifndef LOANY_INT_DB_H
#define LOANY_INT_DB_H

#include <sqlite3.h>

int loany_db_open(const char *filename, sqlite3 **db);

#endif

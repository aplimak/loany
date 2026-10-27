#ifndef LOANY_LOANY_H
#define LOANY_LOANY_H

#include <sqlite3.h>

int loany_init(const char *filename, sqlite3 **db);

#endif

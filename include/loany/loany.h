#ifndef LOANY_H
#define LOANY_H

#include <sqlite3.h>

int loany_init(char *filename, sqlite3 **db);

#endif

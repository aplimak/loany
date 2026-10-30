#ifndef LOANY_INT_DB_H
#define LOANY_INT_DB_H

#include "loany/loany.h"
#include <sqlite3.h>

int loany_db_open(const char *filename, sqlite3 **db);
int loany_db_add_person(sqlite3 *db, loany_person_t *person);
int loany_db_add_people(sqlite3 *db, loany_person_t *const *people, size_t n);

#endif

#include "loany/loany.h"

#include "loany/internal/db.h"

#include <sqlite3.h>

int loany_init(char *filename, sqlite3 **db) {
  return loany_db_open(filename, db);
}

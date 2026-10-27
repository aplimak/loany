#include "loany/loany.h"

#include <sqlite3.h>

int loany_init(char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc != SQLITE_OK) {
    return rc;
  }
  return SQLITE_OK;
}

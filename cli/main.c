#include "loany/loany.h"
#include <sqlite3.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  sqlite3 *db;
  if (loany_init("test.db", &db) != SQLITE_OK) {
    printf("Error opening db: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }
  printf("Opened sqlite db\n");
  return 0;
}

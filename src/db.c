#include "loany/internal/db.h"
#include "loany/loany.h"

#include "generated/sql_str.h"

#include <sqlite3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

int loany_db_open(const char *filename, sqlite3 **db) {
  bool do_rollback = false;
  int rc = sqlite3_open_v2(
      filename, db,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
  if (rc != SQLITE_OK) {
    return rc;
  }

  char *err = 0;

  if ((rc = sqlite3_exec(*db, LOANY_SQL_SCHEMA_PRAGMA, 0, 0, &err)) !=
      SQLITE_OK)
    goto fail;

  if ((rc = sqlite3_exec(*db, "BEGIN IMMEDIATE;", 0, 0, &err)) != SQLITE_OK)
    goto fail;

  do_rollback = true;

  if ((rc = sqlite3_exec(*db, LOANY_SQL_SCHEMA_TABLES_PEOPLE, 0, 0, &err)) !=
      SQLITE_OK) {
    // fprintf(stderr, "error while creating table %s\n", "people");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, LOANY_SQL_SCHEMA_TABLES_LOANS, 0, 0, &err)) !=
      SQLITE_OK) {
    // fprintf(stderr, "error while creating table %s\n", "loans");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, LOANY_SQL_SCHEMA_TABLES_PAYMENTS, 0, 0, &err)) !=
      SQLITE_OK) {
    // fprintf(stderr, "error while creating table %s\n", "payments");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, LOANY_SQL_SCHEMA_INDEXES, 0, 0, &err)) !=
      SQLITE_OK) {
    // fprintf(stderr, "error while creating indexes\n");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, LOANY_SQL_SCHEMA_VIEWS, 0, 0, &err)) !=
      SQLITE_OK) {
    // fprintf(stderr, "error while creating views\n");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, LOANY_SQL_SCHEMA_TRIGGERS, 0, 0, &err)) !=
      SQLITE_OK) {
    // fprintf(stderr, "error while creating triggers\n");
    goto fail;
  }

  if ((rc = sqlite3_exec(*db, "COMMIT;", 0, 0, &err)) != SQLITE_OK)
    goto fail;

  goto done;

fail:
  if (err) {
    // fprintf(stderr, "SQL error: %s\n", err);
    sqlite3_free(err);
  }
  if (do_rollback)
    sqlite3_exec(*db, "ROLLBACK;", 0, 0, NULL);
done:
  return rc;
}

int loany_db_add_person(sqlite3 *db, loany_person_t *person) {
  int rc = 0;
  int frc = 0;
  sqlite3_stmt *st = NULL;

  if ((rc = sqlite3_prepare_v2(db, LOANY_SQL_COMMAND_ADD_PERSON, -1, &st,
                               NULL)) != SQLITE_OK)
    goto done;

  /* name: NOT NULL */
  if ((rc = sqlite3_bind_text(st, 1, person->name, -1, SQLITE_TRANSIENT)) !=
      SQLITE_OK)
    goto clean;

  /* phone: nullable */
  if ((rc = person->phone
                ? sqlite3_bind_text(st, 2, person->phone, -1, SQLITE_TRANSIENT)
                : sqlite3_bind_null(st, 2)) != SQLITE_OK)
    goto clean;

  /* email: nullable */
  if ((rc = person->email
                ? sqlite3_bind_text(st, 3, person->email, -1, SQLITE_TRANSIENT)
                : sqlite3_bind_null(st, 3)) != SQLITE_OK)
    goto clean;

  /* created_at: has default value */
  if ((rc = person->created_at ? sqlite3_bind_int64(st, 4, person->created_at)
                               : sqlite3_bind_null(st, 4)) != SQLITE_OK)
    goto clean;

  if ((rc = sqlite3_step(st)) != SQLITE_ROW)
    goto clean;

  person->id = sqlite3_column_int64(st, 0);
  person->created_at = sqlite3_column_int64(st, 1);

  /* Second step: must reach DONE, otherwise the write is rolled back. */
  if ((rc = sqlite3_step(st)) != SQLITE_DONE)
    goto clean;

  rc = SQLITE_OK;

clean:
  frc = sqlite3_finalize(st);
  if (rc == SQLITE_OK)
    rc = frc;
done:
  return rc;
}

int loany_db_add_people(sqlite3 *db, loany_person_t *const *people, size_t n) {
  if (n == 0)
    return SQLITE_OK;
  if (!people)
    return SQLITE_MISUSE;
  int rc = 0;
  if ((rc = sqlite3_exec(db, "SAVEPOINT loany_db_add_people", NULL, NULL,
                         NULL)) != SQLITE_OK)
    goto done;
  for (size_t i = 0; i < n; i++) {
    if (!people[i]) {
      rc = SQLITE_MISUSE;
      goto fail;
    }
    if ((rc = loany_db_add_person(db, people[i])) != SQLITE_OK) {
      goto fail;
    }
  }
  if ((rc = sqlite3_exec(db, "RELEASE loany_db_add_people", NULL, NULL,
                         NULL)) != SQLITE_OK)
    goto fail;

  goto done;
fail:
  sqlite3_exec(db,
               "ROLLBACK TO loany_db_add_people; RELEASE loany_db_add_people;",
               NULL, NULL, NULL);
done:
  return rc;
}

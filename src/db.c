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

  static const char *cmd_init_indexes =
      "CREATE INDEX IF NOT EXISTS idx_loans_person_id ON loans(person_id);"
      "CREATE INDEX IF NOT EXISTS idx_payments_loan_id ON payments(loan_id);";
  static const char *cmd_init_views =
      "CREATE VIEW IF NOT EXISTS loan_balances AS "
      "SELECT "
      "  l.id, l.person_id, l.direction, l.principal_amount, "
      "  l.description, l.date_issued, l.due_date, "
      "  COALESCE((SELECT SUM(amount) FROM payments "
      "             WHERE loan_id = l.id), 0) AS paid_amount, "
      "  l.principal_amount - COALESCE((SELECT SUM(amount) FROM payments "
      "                               WHERE loan_id = l.id), 0) AS "
      "outstanding, "
      "  CASE WHEN COALESCE((SELECT SUM(amount) FROM payments "
      "                       WHERE loan_id = l.id), 0) >= l.principal_amount "
      "       THEN 'settled' ELSE 'active' END AS status "
      "FROM loans l;";

  static const char *cmd_init_triggers =
      "CREATE TRIGGER IF NOT EXISTS trg_loans_immutable "
      "BEFORE UPDATE ON loans "
      "BEGIN "
      "  SELECT RAISE(ABORT, 'loans are immutable: delete and re-insert'); "
      "END;"

      "CREATE TRIGGER IF NOT EXISTS trg_payments_immutable "
      "BEFORE UPDATE ON payments "
      "BEGIN "
      "  SELECT RAISE(ABORT, 'payments are immutable: delete and re-insert'); "
      "END;";

  char *err = 0;

  if ((rc = sqlite3_exec(*db,
                         "PRAGMA foreign_keys = ON;"
                         "PRAGMA journal_mode = WAL;"
                         "PRAGMA busy_timeout = 5000;",
                         0, 0, &err)) != SQLITE_OK)
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
  if ((rc = sqlite3_exec(*db, cmd_init_indexes, 0, 0, &err)) != SQLITE_OK) {
    // fprintf(stderr, "error while creating indexes\n");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, cmd_init_views, 0, 0, &err)) != SQLITE_OK) {
    // fprintf(stderr, "error while creating views\n");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, cmd_init_triggers, 0, 0, &err)) != SQLITE_OK) {
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
#define SQL_RET " RETURNING id, created_at"
  static const char sql_without_created_at[] =
      "INSERT INTO people (name, phone, email) VALUES (?1, ?2, ?3)" SQL_RET;
  static const char sql_with_created_at[] =
      "INSERT INTO people (name, phone, email, "
      "                    created_at) VALUES (?1, ?2, ?3, ?4)" SQL_RET;

  sqlite3_stmt *st = NULL;
  int rc = sqlite3_prepare_v2(
      db, person->created_at ? sql_with_created_at : sql_without_created_at, -1,
      &st, NULL);
  if (rc != SQLITE_OK)
    goto done;

  /* name: NOT NULL */
  rc = sqlite3_bind_text(st, 1, person->name, -1, SQLITE_TRANSIENT);
  if (rc != SQLITE_OK)
    goto clean;

  /* phone: nullable */
  if (person->phone)
    rc = sqlite3_bind_text(st, 2, person->phone, -1, SQLITE_TRANSIENT);
  else
    rc = sqlite3_bind_null(st, 2);
  if (rc != SQLITE_OK)
    goto clean;

  /* email: nullable */
  if (person->email)
    rc = sqlite3_bind_text(st, 3, person->email, -1, SQLITE_TRANSIENT);
  else
    rc = sqlite3_bind_null(st, 3);
  if (rc != SQLITE_OK)
    goto clean;

  /* created_at: has default value */
  if (person->created_at) {
    rc = sqlite3_bind_int64(st, 4, person->created_at);
    if (rc != SQLITE_OK)
      goto clean;
  }

  rc = sqlite3_step(st);
  if (rc == SQLITE_DONE) {
    person->id = sqlite3_last_insert_rowid(db);
    rc = SQLITE_OK;
    /* p->created_at is NOT known here */
  } else if (rc == SQLITE_ROW) { /* RETURNING emits exactly one row */
    person->id = sqlite3_column_int64(st, 0);
    person->created_at = sqlite3_column_int64(st, 1);
    rc = SQLITE_OK;
  }

clean:
  sqlite3_finalize(st);
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

#include <sqlite3.h>
#include <stdio.h>

int loany_db_open(char *filename, sqlite3 **db) {
  int rc = sqlite3_open_v2(
      filename, db,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
  if (rc != SQLITE_OK) {
    return rc;
  }

  static const char *cmd_init_people =
      "CREATE TABLE IF NOT EXISTS people ("
      "  id INTEGER PRIMARY KEY,"
      "  name TEXT NOT NULL CHECK(length(trim(name)) > 0),"
      "  phone TEXT,"
      "  email TEXT,"
      "  created_at INTEGER NOT NULL DEFAULT (CAST(strftime('%s','now') AS "
      "    INTEGER)) CHECK(created_at > 0)"
      ");";
  static const char *cmd_init_loans =
      "CREATE TABLE IF NOT EXISTS loans ("
      "  id INTEGER PRIMARY KEY,"
      "  person_id INTEGER NOT NULL REFERENCES people(id) ON DELETE RESTRICT,"
      "  direction TEXT NOT NULL CHECK(direction IN ('lent', 'borrowed')),"
      "  principal_amount INTEGER NOT NULL CHECK(principal_amount > 0),"
      "  description TEXT,"
      "  date_issued INTEGER NOT NULL DEFAULT (CAST(strftime('%s','now') AS "
      "    INTEGER)) CHECK(date_issued > 0),"
      "  due_date INTEGER CHECK(due_date IS NULL OR due_date > 0)"
      ");";
  static const char *cmd_init_payments =
      "CREATE TABLE IF NOT EXISTS payments ("
      "  id INTEGER PRIMARY KEY,"
      "  loan_id INTEGER NOT NULL REFERENCES loans(id) ON DELETE RESTRICT,"
      "  amount INTEGER NOT NULL CHECK(amount > 0),"
      "  payment_date INTEGER NOT NULL DEFAULT (CAST(strftime('%s','now') AS "
      "    INTEGER)) CHECK(payment_date > 0),"
      "  note TEXT"
      ");";
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
      "                               WHERE loan_id = l.id), 0) AS outstanding, "
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

  if ((rc = sqlite3_exec(*db, cmd_init_people, 0, 0, &err)) != SQLITE_OK) {
    fprintf(stderr, "error while creating table %s\n", "people");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, cmd_init_loans, 0, 0, &err)) != SQLITE_OK) {
    fprintf(stderr, "error while creating table %s\n", "loans");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, cmd_init_payments, 0, 0, &err)) != SQLITE_OK) {
    fprintf(stderr, "error while creating table %s\n", "payments");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, cmd_init_indexes, 0, 0, &err)) != SQLITE_OK) {
    fprintf(stderr, "error while creating indexes\n");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, cmd_init_views, 0, 0, &err)) != SQLITE_OK) {
    fprintf(stderr, "error while creating views\n");
    goto fail;
  }
  if ((rc = sqlite3_exec(*db, cmd_init_triggers, 0, 0, &err)) != SQLITE_OK) {
    fprintf(stderr, "error while creating triggers\n");
    goto fail;
  }

  if ((rc = sqlite3_exec(*db, "COMMIT;", 0, 0, &err)) != SQLITE_OK)
    goto fail;

  goto done;

fail:
  if (err) {
    fprintf(stderr, "SQL error: %s\n", err);
    sqlite3_free(err);
  }
  sqlite3_exec(*db, "ROLLBACK;", 0, 0, NULL);
done:
  return rc;
}

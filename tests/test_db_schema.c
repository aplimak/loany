#include "loany/loany.h"
#include "test_util.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

/* ---------- helpers ---------- */

static sqlite3 *open_fresh(void) {
  sqlite3 *db = NULL;
  if (loany_init(":memory:", &db) != SQLITE_OK) {
    fprintf(stderr, "FATAL: cannot open fresh in-memory db\n");
    exit(2);
  }
  return db;
}

static long long query_ll(sqlite3 *db, const char *sql) {
  sqlite3_stmt *stmt = NULL;
  long long v = -1;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    return -1;
  if (sqlite3_step(stmt) == SQLITE_ROW)
    v = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return v;
}

static const char *query_text(sqlite3 *db, const char *sql, char *buf,
                              size_t n) {
  sqlite3_stmt *stmt = NULL;
  buf[0] = '\0';
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    return buf;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *val = sqlite3_column_text(stmt, 0);
    if (val) {
      strncpy(buf, (const char *)val, n - 1);
      buf[n - 1] = '\0';
    }
  }
  sqlite3_finalize(stmt);
  return buf;
}

/* Common fixtures */
static void setup_person(sqlite3 *db) {
  T_SQL_OK(db, "INSERT INTO people(name) VALUES ('Alice');");
}
static void setup_loan(sqlite3 *db) {
  setup_person(db);
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'lent', 1000);");
}

/* ---------- schema tests ---------- */

static void test_schema_tables_exist(void) {
  sqlite3 *db = open_fresh();
  T_EQ(query_ll(
           db, "SELECT COUNT(*) FROM sqlite_master "
               "WHERE type='table' AND name IN ('people','loans','payments');"),
       3);
  sqlite3_close(db);
}

static void test_schema_view_exists(void) {
  sqlite3 *db = open_fresh();
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM sqlite_master "
                    "WHERE type='view' AND name='loan_balances';"),
       1);
  sqlite3_close(db);
}

static void test_schema_triggers_exist(void) {
  sqlite3 *db = open_fresh();
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM sqlite_master "
                    "WHERE type='trigger' AND name IN "
                    "('trg_loans_immutable','trg_payments_immutable');"),
       2);
  sqlite3_close(db);
}

static void test_pragma_foreign_keys_on(void) {
  sqlite3 *db = open_fresh();
  T_EQ(query_ll(db, "PRAGMA foreign_keys;"), 1);
  sqlite3_close(db);
}

static void test_idempotent_reopen(void) {
  const char *path = "/tmp/loany_test_reopen.db";
  remove(path);
  remove("/tmp/loany_test_reopen.db-wal");
  remove("/tmp/loany_test_reopen.db-shm");

  sqlite3 *a = NULL;
  T_EQ(loany_init(path, &a), SQLITE_OK);
  if (a)
    sqlite3_close(a);

  sqlite3 *b = NULL;
  T_EQ(loany_init(path, &b), SQLITE_OK);
  if (b)
    sqlite3_close(b);

  remove(path);
  remove("/tmp/loany_test_reopen.db-wal");
  remove("/tmp/loany_test_reopen.db-shm");
}

/* ---------- people ---------- */

static void test_people_insert_ok(void) {
  sqlite3 *db = open_fresh();
  T_SQL_OK(db, "INSERT INTO people(name) VALUES ('Alice');");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM people;"), 1);
  sqlite3_close(db);
}

static void test_people_insert_with_all_fields(void) {
  sqlite3 *db = open_fresh();
  T_SQL_OK(db, "INSERT INTO people(name, phone, email) "
               "VALUES ('Bob', '+1234', 'bob@example.com');");
  char buf[64];
  T_STREQ(
      query_text(db, "SELECT name FROM people WHERE id=1;", buf, sizeof buf),
      "Bob");
  sqlite3_close(db);
}

static void test_people_name_null_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO people(name) VALUES (NULL);");
  sqlite3_close(db);
}

static void test_people_name_empty_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO people(name) VALUES ('');");
  sqlite3_close(db);
}

static void test_people_name_whitespace_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO people(name) VALUES ('   ');");
  sqlite3_close(db);
}

static void test_people_name_padded_ok(void) {
  sqlite3 *db = open_fresh();
  T_SQL_OK(db, "INSERT INTO people(name) VALUES ('  Alice  ');");
  sqlite3_close(db);
}

static void test_people_created_at_default(void) {
  sqlite3 *db = open_fresh();
  T_SQL_OK(db, "INSERT INTO people(name) VALUES ('Alice');");
  T_EQ(query_ll(db, "SELECT created_at > 0 FROM people WHERE id=1;"), 1);
  sqlite3_close(db);
}

/* ---------- loans ---------- */

static void test_loan_lent_ok(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'lent', 10000);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 1);
  sqlite3_close(db);
}

static void test_loan_borrowed_ok(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'borrowed', 5000);");
  sqlite3_close(db);
}

static void test_loan_invalid_direction_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'owe', 1000);");
  sqlite3_close(db);
}

static void test_loan_null_direction_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, NULL, 1000);");
  sqlite3_close(db);
}

static void test_loan_zero_principal_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', 0);");
  sqlite3_close(db);
}

static void test_loan_negative_principal_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', -100);");
  sqlite3_close(db);
}

static void test_loan_fk_missing_person_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (999, 'lent', 1000);");
  sqlite3_close(db);
}

static void test_loan_null_person_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (NULL, 'lent', 1000);");
  sqlite3_close(db);
}

/* ---------- payments ---------- */

static void test_payment_insert_ok(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 500);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM payments;"), 1);
  sqlite3_close(db);
}

static void test_payment_zero_amount_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 0);");
  sqlite3_close(db);
}

static void test_payment_negative_amount_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (1, -50);");
  sqlite3_close(db);
}

static void test_payment_fk_missing_loan_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (999, 100);");
  sqlite3_close(db);
}

/* ---------- immutability ---------- */

static void test_loan_update_blocked(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "UPDATE loans SET principal_amount = 9999 WHERE id = 1;");
  T_EQ(query_ll(db, "SELECT principal_amount FROM loans WHERE id=1;"), 1000);
  sqlite3_close(db);
}

static void test_payment_update_blocked(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 500);");
  T_SQL_FAIL(db, "UPDATE payments SET amount = 999 WHERE id = 1;");
  T_EQ(query_ll(db, "SELECT amount FROM payments WHERE id=1;"), 500);
  sqlite3_close(db);
}

/* ---------- delete / RESTRICT ---------- */

static void test_delete_person_with_loans_blocked(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "DELETE FROM people WHERE id = 1;");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM people;"), 1);
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 1);
  sqlite3_close(db);
}

static void test_delete_loan_with_payments_blocked(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 100);");
  T_SQL_FAIL(db, "DELETE FROM loans WHERE id = 1;");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 1);
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM payments;"), 1);
  sqlite3_close(db);
}

static void test_delete_payment_then_loan_ok(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 100);");
  T_SQL_OK(db, "DELETE FROM payments WHERE id = 1;");
  T_SQL_OK(db, "DELETE FROM loans WHERE id = 1;");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 0);
  sqlite3_close(db);
}

static void test_delete_loan_no_payments_ok(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "DELETE FROM loans WHERE id = 1;");
  sqlite3_close(db);
}

/* ---------- view / status derivation ---------- */

static void test_view_active_no_payments(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  char buf[16];
  T_STREQ(query_text(db, "SELECT status FROM loan_balances WHERE id=1;", buf,
                     sizeof buf),
          "active");
  T_EQ(query_ll(db, "SELECT outstanding FROM loan_balances WHERE id=1;"), 1000);
  T_EQ(query_ll(db, "SELECT paid_amount FROM loan_balances WHERE id=1;"), 0);
  sqlite3_close(db);
}

static void test_view_active_partial_payment(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 400);");
  char buf[16];
  T_STREQ(query_text(db, "SELECT status FROM loan_balances WHERE id=1;", buf,
                     sizeof buf),
          "active");
  T_EQ(query_ll(db, "SELECT outstanding FROM loan_balances WHERE id=1;"), 600);
  sqlite3_close(db);
}

static void test_view_settled_exact(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 400);");
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 600);");
  char buf[16];
  T_STREQ(query_text(db, "SELECT status FROM loan_balances WHERE id=1;", buf,
                     sizeof buf),
          "settled");
  T_EQ(query_ll(db, "SELECT outstanding FROM loan_balances WHERE id=1;"), 0);
  sqlite3_close(db);
}

static void test_view_settled_overpaid(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 1500);");
  char buf[16];
  T_STREQ(query_text(db, "SELECT status FROM loan_balances WHERE id=1;", buf,
                     sizeof buf),
          "settled");
  T_EQ(query_ll(db, "SELECT outstanding FROM loan_balances WHERE id=1;"), -500);
  sqlite3_close(db);
}

static void test_view_reverts_on_payment_delete(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 400);");
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 600);");
  char buf[16];
  T_STREQ(query_text(db, "SELECT status FROM loan_balances WHERE id=1;", buf,
                     sizeof buf),
          "settled");
  T_SQL_OK(db, "DELETE FROM payments WHERE amount = 600;");
  T_STREQ(query_text(db, "SELECT status FROM loan_balances WHERE id=1;", buf,
                     sizeof buf),
          "active");
  T_EQ(query_ll(db, "SELECT outstanding FROM loan_balances WHERE id=1;"), 600);
  sqlite3_close(db);
}

static void test_view_settled_without_payment_rows(void) {
  /* Verifies COALESCE(SUM,0) path when there are zero payments */
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_EQ(query_ll(db, "SELECT paid_amount FROM loan_balances WHERE id=1;"), 0);
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM payments;"), 0);
  sqlite3_close(db);
}

/* ---------- summary ---------- */

static void test_summary_query(void) {
  sqlite3 *db = open_fresh();
  T_SQL_OK(db, "INSERT INTO people(name) VALUES ('Alice');"); /* id 1 */
  T_SQL_OK(db, "INSERT INTO people(name) VALUES ('Bob');");   /* id 2 */

  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'lent', 1000);"); /* id 1 */
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'borrowed', 500);"); /* id 2 */
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (2, 'lent', 200);"); /* id 3 */

  T_SQL_OK(
      db,
      "INSERT INTO payments(loan_id, amount) VALUES (1, 300);"); /* loan1
                                                                    partial */
  T_SQL_OK(
      db,
      "INSERT INTO payments(loan_id, amount) VALUES (3, 200);"); /* loan3
                                                                    settled */

  long long they_owe = query_ll(
      db, "SELECT COALESCE(SUM(CASE WHEN direction='lent' AND status='active' "
          "THEN outstanding ELSE 0 END), 0) FROM loan_balances;");
  long long i_owe = query_ll(
      db,
      "SELECT COALESCE(SUM(CASE WHEN direction='borrowed' AND status='active' "
      "THEN outstanding ELSE 0 END), 0) FROM loan_balances;");
  long long net = query_ll(
      db, "SELECT COALESCE(SUM(CASE "
          "WHEN direction='lent'     AND status='active' THEN  outstanding "
          "WHEN direction='borrowed' AND status='active' THEN -outstanding "
          "ELSE 0 END), 0) FROM loan_balances;");

  T_EQ(they_owe, 700); /* loan1: 1000 - 300 = 700 (loan3 settled, excluded) */
  T_EQ(i_owe, 500);    /* loan2: no payments */
  T_EQ(net, 200);      /* 700 - 500 */
  sqlite3_close(db);
}

/* ---------- dates ---------- */

static void test_date_issued_zero_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(
      db,
      "INSERT INTO loans(person_id, direction, principal_amount, date_issued) "
      "VALUES (1, 'lent', 1000, 0);");
  sqlite3_close(db);
}

static void test_due_date_zero_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(
      db, "INSERT INTO loans(person_id, direction, principal_amount, due_date) "
          "VALUES (1, 'lent', 1000, 0);");
  sqlite3_close(db);
}

static void test_payment_date_zero_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount, payment_date) VALUES "
                 "(1, 100, 0);");
  sqlite3_close(db);
}

static void test_due_date_null_ok(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(
      db, "INSERT INTO loans(person_id, direction, principal_amount, due_date) "
          "VALUES (1, 'lent', 1000, NULL);");
  sqlite3_close(db);
}

/* ---------- integer strictness: people ---------- */

static void test_created_at_text_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO people(name, created_at) VALUES ('A', 'abc');");
  sqlite3_close(db);
}

static void test_created_at_real_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO people(name, created_at) VALUES ('A', 1.5);");
  sqlite3_close(db);
}

static void test_id_string_rejected(void) {
  /* INTEGER PRIMARY KEY (rowid alias) refuses non-integer with SQLITE_MISMATCH
   */
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO people(id, name) VALUES ('abc', 'A');");
  sqlite3_close(db);
}

static void test_id_real_rejected(void) {
  sqlite3 *db = open_fresh();
  T_SQL_FAIL(db, "INSERT INTO people(id, name) VALUES (1.5, 'A');");
  sqlite3_close(db);
}

/* ---------- integer strictness: loans ---------- */

static void test_principal_text_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', 'abc');");
  sqlite3_close(db);
}

static void test_principal_real_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', 1.5);");
  sqlite3_close(db);
}

static void test_principal_blob_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', x'31303030');"); /* BLOB "1000" */
  sqlite3_close(db);
}

static void test_principal_numeric_string_becomes_integer(void) {
  /* Documented behavior: INTEGER affinity losslessly converts numeric strings,
     so '1234' lands as integer 1234. typeof check passes. */
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'lent', '1234');");
  T_EQ(query_ll(db, "SELECT principal_amount FROM loans WHERE id=1;"), 1234);
  T_EQ(query_ll(db, "SELECT typeof(principal_amount) = 'integer' "
                    "FROM loans WHERE id=1;"),
       1);
  sqlite3_close(db);
}

static void test_principal_overflow_rejected(void) {
  /* Larger than int64: SQLite falls back to REAL, typeof catches it */
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', 99999999999999999999);");
  sqlite3_close(db);
}

static void test_date_issued_text_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(
      db,
      "INSERT INTO loans(person_id, direction, principal_amount, date_issued) "
      "VALUES (1, 'lent', 1000, 'abc');");
  sqlite3_close(db);
}

static void test_date_issued_real_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(
      db,
      "INSERT INTO loans(person_id, direction, principal_amount, date_issued) "
      "VALUES (1, 'lent', 1000, 1.5);");
  sqlite3_close(db);
}

static void test_due_date_text_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(
      db, "INSERT INTO loans(person_id, direction, principal_amount, due_date) "
          "VALUES (1, 'lent', 1000, 'abc');");
  sqlite3_close(db);
}

static void test_due_date_real_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(
      db, "INSERT INTO loans(person_id, direction, principal_amount, due_date) "
          "VALUES (1, 'lent', 1000, 1.5);");
  sqlite3_close(db);
}

static void test_direction_blob_rejected(void) {
  /* BLOB is never coerced to TEXT; IN check rejects it */
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, x'6c656e74', 1000);"); /* BLOB "lent" */
  sqlite3_close(db);
}

/* ---------- integer strictness: payments ---------- */

static void test_amount_text_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 'abc');");
  sqlite3_close(db);
}

static void test_amount_real_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (1, 1.5);");
  sqlite3_close(db);
}

static void test_amount_numeric_string_becomes_integer(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "INSERT INTO payments(loan_id, amount) VALUES (1, '250');");
  T_EQ(query_ll(db, "SELECT amount FROM payments WHERE id=1;"), 250);
  T_EQ(query_ll(db,
                "SELECT typeof(amount) = 'integer' FROM payments WHERE id=1;"),
       1);
  sqlite3_close(db);
}

static void test_amount_overflow_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) "
                 "VALUES (1, 99999999999999999999);");
  sqlite3_close(db);
}

static void test_payment_date_text_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount, payment_date) "
                 "VALUES (1, 100, 'abc');");
  sqlite3_close(db);
}

static void test_payment_date_real_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount, payment_date) "
                 "VALUES (1, 100, 1.5);");
  sqlite3_close(db);
}

/* ---------- FK-column integer strictness (isolated from FK enforcement)
 * ---------- FK enforcement is disabled inside these tests on purpose. Without
 * the typeof() CHECK on person_id / loan_id, the row lands as text/real and the
 * test fails. With the CHECK, the insert is aborted. This is what proves the
 * CHECK is load-bearing rather than decorative.
 */

static void test_person_id_text_rejected_by_typeof(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "PRAGMA foreign_keys = OFF;");
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES ('abc', 'lent', 1000);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 0);
  sqlite3_close(db);
}

static void test_person_id_real_rejected_by_typeof(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "PRAGMA foreign_keys = OFF;");
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1.5, 'lent', 1000);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 0);
  sqlite3_close(db);
}

static void test_person_id_blob_rejected_by_typeof(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "PRAGMA foreign_keys = OFF;");
  /* BLOB x'31' = text "1" as bytes; FK OFF, only typeof catches it */
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (x'31', 'lent', 1000);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 0);
  sqlite3_close(db);
}

static void test_loan_id_text_rejected_by_typeof(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "PRAGMA foreign_keys = OFF;");
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES ('abc', 100);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM payments;"), 0);
  sqlite3_close(db);
}

static void test_loan_id_real_rejected_by_typeof(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "PRAGMA foreign_keys = OFF;");
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (1.5, 100);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM payments;"), 0);
  sqlite3_close(db);
}

static void test_loan_id_blob_rejected_by_typeof(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_OK(db, "PRAGMA foreign_keys = OFF;");
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (x'31', 100);");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM payments;"), 0);
  sqlite3_close(db);
}

static void test_principal_float_string_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_person(db);
  /* '12.5' → INTEGER affinity → REAL 12.5 → typeof check must reject */
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', '12.5');");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 0);
  sqlite3_close(db);
}

static void test_amount_float_string_rejected(void) {
  sqlite3 *db = open_fresh();
  setup_loan(db);
  T_SQL_FAIL(db, "INSERT INTO payments(loan_id, amount) VALUES (1, '12.5');");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM payments;"), 0);
  sqlite3_close(db);
}

static void test_principal_exponent_string_integral_becomes_integer(void) {
  /* '1e5' == 100000.0, losslessly integral → INTEGER affinity stores as 100000.
   * This is correct behavior, not a bypass. Asserts the conversion path. */
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'lent', '1e5');");
  T_EQ(query_ll(db, "SELECT principal_amount FROM loans WHERE id=1;"), 100000);
  T_EQ(query_ll(db, "SELECT typeof(principal_amount) = 'integer' "
                    "FROM loans WHERE id=1;"),
       1);
  sqlite3_close(db);
}

static void test_principal_real_string_integral_becomes_integer(void) {
  /* '12.0' == 12.0, losslessly integral → stored as INTEGER 12.
   * Same rule as '1e5': the decimal point doesn't matter if the value is whole.
   */
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_OK(db, "INSERT INTO loans(person_id, direction, principal_amount) "
               "VALUES (1, 'lent', '12.0');");
  T_EQ(query_ll(db, "SELECT principal_amount FROM loans WHERE id=1;"), 12);
  T_EQ(query_ll(db, "SELECT typeof(principal_amount) = 'integer' "
                    "FROM loans WHERE id=1;"),
       1);
  sqlite3_close(db);
}

static void test_principal_exponent_string_nonintegral_rejected(void) {
  /* '1e-1' == 0.1, not losslessly integral → stays REAL.
   * typeof() guard must reject. If someone removes the CHECK, this fails. */
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', '1e-1');");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 0);
  sqlite3_close(db);
}

static void test_principal_exponent_string_overflow_rejected(void) {
  /* '1e19' == 10^19, exceeds INT64_MAX (~9.22e18). Cannot be stored as
   * integer → stays REAL. typeof() guard must reject. Catches a weakened
   * CHECK and also documents overflow handling. */
  sqlite3 *db = open_fresh();
  setup_person(db);
  T_SQL_FAIL(db, "INSERT INTO loans(person_id, direction, principal_amount) "
                 "VALUES (1, 'lent', '1e19');");
  T_EQ(query_ll(db, "SELECT COUNT(*) FROM loans;"), 0);
  sqlite3_close(db);
}

/* ---------- main ---------- */

int main(void) {
  printf("loany test suite\n\n");

  RUN(test_schema_tables_exist);
  RUN(test_schema_view_exists);
  RUN(test_schema_triggers_exist);
  RUN(test_pragma_foreign_keys_on);
  RUN(test_idempotent_reopen);

  RUN(test_people_insert_ok);
  RUN(test_people_insert_with_all_fields);
  RUN(test_people_name_null_rejected);
  RUN(test_people_name_empty_rejected);
  RUN(test_people_name_whitespace_rejected);
  RUN(test_people_name_padded_ok);
  RUN(test_people_created_at_default);

  RUN(test_loan_lent_ok);
  RUN(test_loan_borrowed_ok);
  RUN(test_loan_invalid_direction_rejected);
  RUN(test_loan_null_direction_rejected);
  RUN(test_loan_zero_principal_rejected);
  RUN(test_loan_negative_principal_rejected);
  RUN(test_loan_fk_missing_person_rejected);
  RUN(test_loan_null_person_rejected);

  RUN(test_payment_insert_ok);
  RUN(test_payment_zero_amount_rejected);
  RUN(test_payment_negative_amount_rejected);
  RUN(test_payment_fk_missing_loan_rejected);

  RUN(test_loan_update_blocked);
  RUN(test_payment_update_blocked);

  RUN(test_delete_person_with_loans_blocked);
  RUN(test_delete_loan_with_payments_blocked);
  RUN(test_delete_payment_then_loan_ok);
  RUN(test_delete_loan_no_payments_ok);

  RUN(test_view_active_no_payments);
  RUN(test_view_active_partial_payment);
  RUN(test_view_settled_exact);
  RUN(test_view_settled_overpaid);
  RUN(test_view_reverts_on_payment_delete);
  RUN(test_view_settled_without_payment_rows);

  RUN(test_summary_query);

  RUN(test_date_issued_zero_rejected);
  RUN(test_due_date_zero_rejected);
  RUN(test_payment_date_zero_rejected);
  RUN(test_due_date_null_ok);

  /* integer strictness — people */
  RUN(test_created_at_text_rejected);
  RUN(test_created_at_real_rejected);
  RUN(test_id_string_rejected);
  RUN(test_id_real_rejected);

  /* integer strictness — loans */
  RUN(test_principal_text_rejected);
  RUN(test_principal_real_rejected);
  RUN(test_principal_blob_rejected);
  RUN(test_principal_numeric_string_becomes_integer);
  RUN(test_principal_overflow_rejected);
  RUN(test_date_issued_text_rejected);
  RUN(test_date_issued_real_rejected);
  RUN(test_due_date_text_rejected);
  RUN(test_due_date_real_rejected);
  RUN(test_direction_blob_rejected);

  /* integer strictness — payments */
  RUN(test_amount_text_rejected);
  RUN(test_amount_real_rejected);
  RUN(test_amount_numeric_string_becomes_integer);
  RUN(test_amount_overflow_rejected);
  RUN(test_payment_date_text_rejected);
  RUN(test_payment_date_real_rejected);

  /* FK-column integer strictness isolated from FK enforcement */
  RUN(test_person_id_text_rejected_by_typeof);
  RUN(test_person_id_real_rejected_by_typeof);
  RUN(test_person_id_blob_rejected_by_typeof);
  RUN(test_loan_id_text_rejected_by_typeof);
  RUN(test_loan_id_real_rejected_by_typeof);
  RUN(test_loan_id_blob_rejected_by_typeof);

  /* numeric-string near-misses that must be rejected */
  RUN(test_principal_float_string_rejected);
  RUN(test_amount_float_string_rejected);
  RUN(test_principal_exponent_string_integral_becomes_integer);
  RUN(test_principal_real_string_integral_becomes_integer);
  RUN(test_principal_exponent_string_nonintegral_rejected);
  RUN(test_principal_exponent_string_overflow_rejected);

  return SUMMARY();
}

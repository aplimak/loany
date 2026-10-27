#ifndef LOANY_TEST_UTIL_H
#define LOANY_TEST_UTIL_H

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_run = 0;
static int g_fail = 0;
static const char *g_test = NULL;

#define T_ASSERT(cond)                                                         \
  do {                                                                         \
    g_run++;                                                                   \
    if (!(cond)) {                                                             \
      g_fail++;                                                                \
      fprintf(stderr, "  X %s :: %s:%d\n    %s\n", g_test, __FILE__, __LINE__, \
              #cond);                                                          \
    }                                                                          \
  } while (0)

#define T_EQ(actual, expected)                                                 \
  do {                                                                         \
    g_run++;                                                                   \
    long long _a = (long long)(actual);                                        \
    long long _e = (long long)(expected);                                      \
    if (_a != _e) {                                                            \
      g_fail++;                                                                \
      fprintf(stderr, "  X %s :: %s:%d\n    %s == %lld, expected %lld\n",      \
              g_test, __FILE__, __LINE__, #actual, _a, _e);                    \
    }                                                                          \
  } while (0)

#define T_STREQ(actual, expected)                                              \
  do {                                                                         \
    g_run++;                                                                   \
    const char *_a = (actual);                                                 \
    const char *_e = (expected);                                               \
    if (_a == NULL || _e == NULL || strcmp(_a, _e) != 0) {                     \
      g_fail++;                                                                \
      fprintf(stderr, "  X %s :: %s:%d\n    %s = \"%s\", expected \"%s\"\n",   \
              g_test, __FILE__, __LINE__, #actual, _a ? _a : "(null)",         \
              _e ? _e : "(null)");                                             \
    }                                                                          \
  } while (0)

#define T_SQL_OK(db, sql)                                                      \
  do {                                                                         \
    g_run++;                                                                   \
    int _rc = sqlite3_exec((db), (sql), 0, 0, NULL);                           \
    if (_rc != SQLITE_OK) {                                                    \
      g_fail++;                                                                \
      fprintf(stderr, "  X %s :: %s:%d\n    SQL failed: %s\n    err: %s\n",    \
              g_test, __FILE__, __LINE__, (sql), sqlite3_errmsg(db));          \
    }                                                                          \
  } while (0)

#define T_SQL_FAIL(db, sql)                                                    \
  do {                                                                         \
    g_run++;                                                                   \
    int _rc = sqlite3_exec((db), (sql), 0, 0, NULL);                           \
    if (_rc == SQLITE_OK) {                                                    \
      g_fail++;                                                                \
      fprintf(                                                                 \
          stderr,                                                              \
          "  X %s :: %s:%d\n    expected failure, but SQL succeeded: %s\n",    \
          g_test, __FILE__, __LINE__, (sql));                                  \
    }                                                                          \
  } while (0)

#define RUN(fn)                                                                \
  do {                                                                         \
    g_test = #fn;                                                              \
    printf("  %s\n", #fn);                                                     \
    fn();                                                                      \
  } while (0)

#define SUMMARY()                                                              \
  (printf("\n%d assertions, %d failed\n", g_run, g_fail),                      \
   g_fail == 0 ? EXIT_SUCCESS : EXIT_FAILURE)

#endif

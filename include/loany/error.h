#ifndef LOANY_ERROR_H
#define LOANY_ERROR_H

#include <stddef.h>

typedef enum {
  LOANY_OK = 0,
  LOANY_ERR_NOMEM,
  LOANY_ERR_INVAL,
  LOANY_ERR_IO,
  LOANY_ERR_NOTFOUND,
  LOANY_ERR_EXISTS,
  LOANY_ERR_STATE,
  LOANY_ERR_SQLITE,
  LOANY_ERR_OLD_SQLITE,
  LOANY_ERR__COUNT
} loany_error;

typedef struct sqlite3 sqlite3;

/* Static description of a loany_error. For LOANY_ERR_SQLITE this returns a
 * generic "sqlite error" string. */
const char *loany_strerror(loany_error err);

/*Dynamic sqlite error message. */
const char *loany_sql_strerror(sqlite3 *db);

/*Dynamic sqlite error code. */
int loany_sql_errcode(sqlite3 *db);
#endif

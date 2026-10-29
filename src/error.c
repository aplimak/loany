#include "loany/error.h"

#include <sqlite3.h>

static const char *const k_loany_msgs[LOANY_ERR__COUNT] = {
    [LOANY_OK] = "success",
    [LOANY_ERR_NOMEM] = "out of memory",
    [LOANY_ERR_INVAL] = "invalid argument",
    [LOANY_ERR_IO] = "I/O error",
    [LOANY_ERR_NOTFOUND] = "not found",
    [LOANY_ERR_EXISTS] = "already exists",
    [LOANY_ERR_STATE] = "invalid library state",
    [LOANY_ERR_SQLITE] = "sqlite error",
};

const char *loany_strerror(loany_error err) {
  if ((unsigned)err < LOANY_ERR__COUNT)
    return k_loany_msgs[err];
  return "unknown error";
}

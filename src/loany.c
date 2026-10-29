#include "loany/loany.h"

#include "loany/error.h"
#include "loany/internal/db.h"

#include <sqlite3.h>
#include <stdlib.h>

loany_error loany_init(const char *filename,
                       void (*update_callback)(struct loany_state *state),
                       loany_state_t **state) {
  *state = NULL;
  int runtime_ver = sqlite3_libversion_number();
  // 3.35.0 is represented as 3035000
  if (runtime_ver < 3035000) {
    return LOANY_ERR_OLD_SQLITE;
  }

  loany_state_t *s = calloc(1, sizeof(*s));
  if (!s) {
    return LOANY_ERR_NOMEM;
  }
  *state = s;

  s->update_callback = update_callback;
  int sqlite_rc = loany_db_open(filename, &s->db);
  if (sqlite_rc != SQLITE_OK)
    return LOANY_ERR_SQLITE;
  return LOANY_OK;
}

void loany_close(loany_state_t **statep) {
  if (!statep || !*statep)
    return;
  loany_state_t *s = *statep;
  if (s->db) {
    sqlite3_close_v2(s->db);
    s->db = NULL;
  }

  free(s);
  *statep = NULL;
}

loany_error loany_add_person(loany_state_t *state, loany_person_t *person) {
  int sqlite_rc = loany_db_add_person(state->db, person);
  if (sqlite_rc != SQLITE_OK)
    return LOANY_ERR_SQLITE;
  if (state->update_callback)
    state->update_callback(state);
  return LOANY_OK;
}

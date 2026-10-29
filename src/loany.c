#include "loany/loany.h"

#include "loany/error.h"
#include "loany/internal/db.h"

#include <sqlite3.h>
#include <stdlib.h>

loany_error loany_init(const char *filename,
                       void (*update_callback)(struct loany_state *state),
                       loany_state_t **state) {
  *state = malloc(sizeof(loany_state_t));
  (*state)->update_callback = update_callback;
  int sqlite_rc = loany_db_open(filename, &(*state)->db);
  if (sqlite_rc != SQLITE_OK)
    return LOANY_ERR_SQLITE;
  return LOANY_OK;
}

void loany_close(loany_state_t *state) { sqlite3_close(state->db); }

loany_error loany_add_person(loany_state_t *state, loany_person_t *person) {
  int sqlite_rc = loany_db_add_person(state->db, person);
  if (sqlite_rc != SQLITE_OK)
    return LOANY_ERR_SQLITE;
  if (state->update_callback)
    state->update_callback(state);
  return LOANY_OK;
}

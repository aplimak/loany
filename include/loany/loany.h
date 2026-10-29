#ifndef LOANY_LOANY_H
#define LOANY_LOANY_H

#include "loany/error.h"
#include <sqlite3.h>
#include <stdint.h>

typedef struct loany_state {
  sqlite3 *db;
  void (*update_callback)(struct loany_state *state);
} loany_state_t;

typedef struct loany_person {
  sqlite3_int64 id;
  char *name;
  char *phone;
  char *email;
  sqlite3_int64 created_at;
} loany_person_t;

/**
 * Open or create a database and initialize the loany schema.
 * The state must be closed even on fail.
 */
loany_error loany_init(const char *filename,
                       void (*update_callback)(struct loany_state *state),
                       loany_state_t *state);

/**
 * Close the database. and clean up the state
 */
void loany_close(loany_state_t *statep);

/**
 * Add single person to the database.
 */
loany_error loany_add_person(loany_state_t *state, loany_person_t *person);

#endif

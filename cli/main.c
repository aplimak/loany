#include "loany/error.h"
#include "loany/loany.h"
#include <sqlite3.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  loany_state_t *state;
  int exit_code = loany_init("test.db", NULL, &state);
  if (exit_code != LOANY_OK) {
    fprintf(stderr, "%s", loany_strerror(exit_code));
    if (exit_code == LOANY_ERR_SQLITE) {
      fprintf(stderr, ": %s\n", sqlite3_errmsg(state->db));
      exit_code = sqlite3_errcode(state->db);
    } else {
      fprintf(stderr, "\n");
    }
    goto clean;
  }
  printf("Opened sqlite db\n");

clean:
  loany_close(state);
  return exit_code;
}

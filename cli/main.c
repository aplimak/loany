#include "loany/error.h"
#include "loany/loany.h"
#include <sqlite3.h>
#include <stdio.h>

#define HANDLE_ERR(m)                                                          \
  exit_code = m;                                                               \
  if (exit_code != LOANY_OK) {                                                 \
    fprintf(stderr, "%s", loany_strerror(exit_code));                          \
    if (exit_code == LOANY_ERR_SQLITE) {                                       \
      fprintf(stderr, ": %s\n", sqlite3_errmsg(state->db));                    \
      exit_code = sqlite3_errcode(state->db);                                  \
    } else {                                                                   \
      fprintf(stderr, "\n");                                                   \
    }                                                                          \
    goto clean;                                                                \
  }

void update_hook(loany_state_t *state) { printf("Received update signal\n"); }

int main(int argc, char *argv[]) {
  loany_state_t *state;
  int exit_code = 0;
  HANDLE_ERR(loany_init("test.db", update_hook, &state));
  printf("Opened sqlite db\n");
  loany_person_t person = {
      .name = "Test", .phone = "test phone", .email = "test@email.com"};
  HANDLE_ERR(loany_add_person(state, &person));

clean:
  loany_close(state);
  return exit_code;
}

#include "weld.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct weld_config weldcfg;

int weld_main(struct weld_config cfg) {
  weldcfg = cfg;

  return 0;
}

struct weld_config weld_config_from_env(void) {
  struct weld_config cfg;
  memset(&cfg, 0, sizeof(cfg));

  return cfg;
}

int weld_commtok(char *dst, const char *src, size_t len) {
  assert(len);

  size_t lenm1 = len - 1;

  int i = 0;
  char prev = '\0';
  while (i < lenm1 && *src &&
         (prev == WELD_COMM_ESCAPE || *src != WELD_COMM_TERM)) {
    i++;
    if (prev != WELD_COMM_ESCAPE && *src == WELD_COMM_ESCAPE) {
      prev = *src;
      src++;
      continue;
    }

    prev = *src;
    *dst = *src;
    dst++;
    src++;
  }

  *dst = '\0';

  if (i >= lenm1) {
    fprintf(stderr, "The supplied buffer did not provide enough memory to fit "
                    "the entire token!\n");
    return -1;
  }

  return i;
}

struct weld_comm weld_commfrom(const char *line) {
  struct weld_comm comm;
  memset(&comm, 0, sizeof(comm));
  comm.ok = -1;
  comm.type = WELD_COMM_NOP;

  return comm;
}

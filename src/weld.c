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

char *weld_commpath(char *dst, const char *src, size_t len) {
  assert(len);

  char *dst_start = dst;

  size_t i = 0;
  char prev = '\0';
  while (i < (len - 1) && *src &&
         (prev == WELD_COMM_ESCAPE || *src != WELD_COMM_TERM)) {
    if (prev != WELD_COMM_ESCAPE && *src == WELD_COMM_ESCAPE) {
      prev = *src;
      src++;
      continue;
    }

    prev = *src;
    *dst = *src;
    dst++;
    src++;
    i++;
  }

  *dst = '\0';

  return dst_start;
}

struct weld_comm weld_commfrom(const char *line) {
  struct weld_comm comm;
  memset(&comm, 0, sizeof(comm));
  comm.ok = -1;
  comm.type = WELD_COMM_NOP;

  return comm;
}

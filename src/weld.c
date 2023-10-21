#include "weld.h"
#include <assert.h>
#include <ctype.h>
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
  size_t read = 0;
  const size_t typebuflen = 2;
  char typebuf[typebuflen];
  memset(typebuf, 0, typebuflen);

  char srcbuf[WELD_PATH_MAX];
  memset(srcbuf, 0, WELD_PATH_MAX);

  char dstbuf[WELD_PATH_MAX];
  memset(dstbuf, 0, WELD_PATH_MAX);

  struct weld_comm comm;
  memset(&comm, 0, sizeof(comm));
  comm.ok = -1;
  comm.type = WELD_COMM_NOP;

  // decide if this is a comment
  {
    size_t i = 0;
    while (line[i] && isspace(line[i])) {
      i++;
    }
    if (line[i] == WELD_COMM_COMMENT) {
      goto SKIP_COMMENT;
    }
  }

  // not a comment, read all required args
  read = weld_commtok(typebuf, line, typebuflen);
  if (read == -1) {
    goto FAIL;
  }

  switch (typebuf[0]) {
  case WELD_COMM_SYMLINK:
    comm.type = WELD_COMM_SYMLINK;
    break;
  default:
    fprintf(stderr, "Unknown command type: '%c'\n", typebuf[0]);
    goto FAIL;
  }

SKIP_COMMENT:
  comm.ok = 0;
FAIL:
  return comm;
}

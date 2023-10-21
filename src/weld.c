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
  if (weldcfg.verbose) {
    fprintf(stderr, "[commtok] Parsing '%s'. max len %ld\n", src, len);
  }

  char *dst_start = dst;

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

  if (weldcfg.verbose) {
    fprintf(stderr,
            "[commtok] Result: '%s'. %d bytes read. Reamining source: '%s'\n",
            dst_start, i, src);
  }

  if (*src && *src != WELD_COMM_TERM) {
    fprintf(stderr, "The supplied buffer did not provide enough memory to fit "
                    "the entire token!\n");
    return -1;
  }

  // advance past the separator
  if (*src == WELD_COMM_TERM) {
    i++;
  }

  return i;
}

struct weld_comm weld_commfrom(const char *line) {
  size_t read = 0;
  const size_t typebuflen = 3;
  char typebuf[typebuflen];
  memset(typebuf, 0, typebuflen);

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
  if (read <= 0) {
    goto FAIL;
  }

  switch (typebuf[0]) {
  case WELD_COMM_SYMLINK:
    comm.type = WELD_COMM_SYMLINK;
    line += read;
    read = weld_commtok(comm.src, line, WELD_PATH_MAX);
    if (read <= 0) {
      goto FAIL;
    }

    line += read;
    read = weld_commtok(comm.dst, line, WELD_PATH_MAX);
    if (read <= 0) {
      goto FAIL;
    }

    if (weldcfg.verbose) {
      fprintf(stderr, "[commfrom] read symlink with src: '%s' and dst: '%s'\n",
              comm.src, comm.dst);
    }
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

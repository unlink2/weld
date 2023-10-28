#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "weld.h"
#include <unistd.h>

#define WELD_NAME "weld"
#define WELD_VER "0.0.1"
#define WELD_OPTS "hvVpdcfe"

void weld_help(void) {
  printf("%s\n", WELD_NAME);
  printf("Usage %s [-%s] [commands...]\n\n", WELD_NAME, WELD_OPTS);
  printf("\t-h\tdisplay this help and exit\n");
  printf("\t-V\tdisplay version info and exit\n");
  printf("\t-v\tverbose output\n");
  printf("\t-p\tcreate directories for destination if they are missing\n");
  printf("\t-d\tperform a dry run (print actions but do not act)\n");
  printf("\t-c\tconfirm before each action is taken\n");
  printf("\t-f\tforce creation even if the destination path already exists\n");
  printf("\t-e\tperform shell word expansion on input strings\n");
  printf("\t-s\tskip errors and process next command\n");
}

void weld_version(void) { printf("%s version %s\n", WELD_NAME, WELD_VER); }

void weld_getopt(int argc, char **argv, struct weld_config *cfg) {
  int c = 0;
  while ((c = getopt(argc, argv, WELD_OPTS)) != -1) {
    switch (c) {
    case 'h':
      weld_help();
      exit(0);
      break;
    case 'V':
      weld_version();
      exit(0);
      break;
    case 'v':
      cfg->verbose = true;
      break;
    case 'p':
      cfg->mkdirs = true;
      break;
    case 'd':
      cfg->dry = true;
      break;
    case 'c':
      cfg->confirm = true;
      break;
    case 'f':
      cfg->force = true;
      break;
    case 'e':
      cfg->expand = true;
      break;
    case 's':
      cfg->skip_errors = true;
      break;
    default:
      printf("%s: invalid option '%c'\nTry '%s -h' for more information.\n",
             WELD_NAME, c, WELD_NAME);
      exit(-1);
      break;
    }
  }

  cfg->argc = argc - optind;
  cfg->argv = argv + optind;
}

int main(int argc, char **argv) {
  // map args to cfg here
  struct weld_config cfg = weld_config_from_env();
  weld_getopt(argc, argv, &cfg);

  int res = weld_main(cfg);

  return res;
}

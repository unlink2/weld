#include "weld.h"
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

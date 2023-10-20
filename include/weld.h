#ifndef WELD_H__
#define WELD_H__

#include <stdbool.h>

struct weld_config {
  bool verbose;

  // run dry mode only when this is true
  // if dry is true it will only display a
  // preview of actions that will be taken
  bool dry;
};

// global cfg
extern struct weld_config weldcfg;

int weld_main(struct weld_config cfg);

struct weld_config weld_config_from_env(void);

#endif

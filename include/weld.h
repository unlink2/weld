#ifndef WELD_H__
#define WELD_H__

#include <stdbool.h>
#include <stddef.h>

#ifdef __linux__
#include <linux/limits.h>
#define WELD_PATH_MAX PATH_MAX
#else
// TODO: get limits for other systems
#define WELD_PATH_MAX 4096
#endif

#define WELD_COMM_TERM ':'
#define WELD_COMM_ESCAPE '\\'

struct weld_config {
  bool verbose;

  // run dry mode only when this is true
  // if dry is true it will only display a
  // preview of actions that will be taken
  bool dry;

  // if not in dry mode prompt the user to confirm before taking
  // a destructive action (e.g. before starting creation, or before overwriting
  // an existing link or file)
  bool confirm;

  // create directory tree if it is not present
  bool mkdirs;

  // replace exiting links or files
  bool replace;
};

enum weld_comms {
  // commands return a no operation if the line begins with '#'
  WELD_COMM_NOP = 0,
  WELD_COMM_SYMLINK = 's'
};

struct weld_comm {
  int ok;
  enum weld_comms type;
  union {
    char src[WELD_PATH_MAX];
    char dst[WELD_PATH_MAX];
  };
};

// global cfg
extern struct weld_config weldcfg;

int weld_main(struct weld_config cfg);

struct weld_config weld_config_from_env(void);

struct weld_comm weld_commfrom(const char *line);

char *weld_commpath(char *dst, const char *src, size_t len);

#endif

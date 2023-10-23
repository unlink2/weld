#ifndef WELD_H__
#define WELD_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __linux__
#include <linux/limits.h>
#define WELD_PATH_MAX PATH_MAX
#else
// TODO: get limits for other systems
#define WELD_PATH_MAX 4096
#endif

#define WELD_BUF_MAX PATH_MAX * 3

#define WELD_COMM_TERM ':'
#define WELD_COMM_ESCAPE '\\'
#define WELD_COMM_COMMENT '#'

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define weldin stdin
#define weldout stdout
#define welderr stderr

struct weld_config {
  char **argv;
  int argc;

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
  bool force;

  // calls wordexp(3) on each line read by the input
  // this allows the use of shell-variables in the
  // declaration
  bool expand;
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
    // type symlink
    struct {
      char src[WELD_PATH_MAX];
      char dst[WELD_PATH_MAX];
    };
  };
};

// file stat results
struct weld_stat {
  int ok;
  // ptr to path in comm
  const char *path;
  bool exists;
  bool access;
  int mod;
};

// result of chk
// this determines what running a command will do
struct weld_commchk {
  int ok;
  struct weld_comm *comm;
};

// global cfg
extern struct weld_config weldcfg;

int weld_main(struct weld_config cfg);

int weld_commnext(void);

struct weld_config weld_config_from_env(void);

// calls wordexp(3) on the input line.
// Returns an array of strings.
// The resulting strings are heap allocated and needs to be cleaned up
// using free(2)
char **weld_wordexp(const char *line, size_t *len);
void weld_wordexp_free(char **lines, size_t len);

// creates a command from a line
struct weld_comm weld_commfrom(const char *line);

// writes the next token into dst and returns the number of bytes read
// returns -1 on error (e.g. if dst's size is insufficient)
int weld_commtok(char *dst, const char *src, size_t len);

#endif

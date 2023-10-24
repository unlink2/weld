#ifndef WELD_H__
#define WELD_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __linux__
#include <linux/limits.h>
#define WELD_PATH_MAX PATH_MAX
#else
#define WELD_PATH_MAX 4096
#endif

// platform specific types and macros
// try to wrap posix types and calls here
// to allow the possibility of *maybe* supporting
// non-posix systems in the future
#ifdef __unix__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef mode_t weld_st_mode;

#define WELD_S_ISREG(m) S_ISREG(m)
#define WELD_S_ISDIR(m) S_ISDIR(m)
#define WELD_S_ISCHR(m) S_ISCHR(m)
#define WELD_S_ISBLK(m) S_SIBLK(m)
#define WELD_S_ISFIFO(m) S_ISFIFO(m)
#define WELD_IS_ISLIN(M) S_ISLINK(m)
#define WELD_S_ISSOCK(m) S_ISSOCK(m)

#else
#error "Platform is not supported"
#endif

#define WELD_BUF_MAX (WELD_PATH_MAX * 3)

#define WELD_COMM_TERM ':'
#define WELD_COMM_ESCAPE '\\'
#define WELD_COMM_COMMENT '#'

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define weldin stdin
#define weldout stdout
#define welderr stderr

// Config variables
#define WELD_CFG_FMT_OK "\x1B[32m"
#define WELD_CFG_FMT_WARN "\x1B[33m"
#define WELD_CFG_FMT_ERR "\x1B[31m"
#define WELD_CFG_FMT_RESET "\x1B[0m"

// format macros
#define WELD_FMT(f, fmt)                                                       \
  if (isatty(f) && weldcfg.color) {                                            \
    fprintf((f), "%s", (fmt));                                                 \
  }

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

  bool color;
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
  weld_st_mode st_mode;
};

// result of chk
// this determines what running a command will do
// this type is only valid as long as comm is valid
struct weld_commchk {
  int ok;
  struct weld_comm *comm;
  union {
    // type symlink
    struct {
      struct weld_stat src_stat;
      struct weld_stat dst_stat;
    };
  };
};

// global cfg
extern struct weld_config weldcfg;

int weld_main(struct weld_config cfg);

int weld_commnext(void);

struct weld_config weld_config_from_env(void);
struct weld_commchk weld_commchk(struct weld_comm *comm);
struct weld_stat weld_stat(const char *path);
size_t weld_readlink(const char *path, char *buf, size_t bufsize);

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

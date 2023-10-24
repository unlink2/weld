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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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
#define WELD_CFG_FMT_GREEN "\x1B[32m"
#define WELD_CFG_FMT_YELLOW "\x1B[33m"
#define WELD_CFG_FMT_RED "\x1B[31m"
#define WELD_CFG_FMT_MAGENTA "\x1B[35m"
#define WELD_CFG_FMT_CYAN "\x1B[36m"
#define WELD_CFG_FMT_BLUE "\x1B[34m"
#define WELD_CFG_FMT_RESET "\x1B[0m"

// format macros
#define WELD_FMT(f, fmt)                                                       \
  if (isatty(fileno(f)) && weldcfg.color) {                                    \
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
  // stat will only be set if exists is true and ok is 0
  struct stat st;
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

// reads next command from stdin file 
// returns 0 on success, -1 on failure, and 1 if successful 
// but no more input is present
int weld_fcommnext(void);

// executes a command
// returns 0 on success and -1 on failure 
int weld_commnext(char *buf, size_t buflen);

struct weld_config weld_config_from_env(void);
struct weld_commchk weld_commchk(struct weld_comm *comm);
struct weld_stat weld_stat(const char *path);

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

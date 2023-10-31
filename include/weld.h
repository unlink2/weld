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

#define WELD_VERBOSE "WELD_VERBOSE"

#define WELD_BUF_MAX (WELD_PATH_MAX * 3)

#define WELD_COMM_TERM ':'
#define WELD_COMM_ESCAPE '\\'
#define WELD_COMM_COMMENT '#'

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/**
 * these files are initied to NULL
 * if they are not mapped to anything else they will
 * be mapped to stdin, stdout and stderr respectively
 * upon calling weld_main or weld_init
 */
extern FILE *weldin;
extern FILE *weldout;
extern FILE *welderr;

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
  // argv and argc are used
  // as command strings
  // they are not interpreted as cli flags
  // the parsing of cli flags is left up to the caller
  // of weld_main
  char **argv;
  int argc;

  int mkdir_mode;

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

  // skip errors
  // and continue to next command anyway
  bool skip_errors;

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

// global cfg
extern struct weld_config weldcfg;

void weld_init(struct weld_config cfg);
int weld_main(struct weld_config cfg);

// reads next command from stdin file
// returns 0 on success, -1 on failure, and 1 if successful
// but no more input is present
int weld_fcommnext(void);

// executes a command
// returns 0 on success and -1 on failure
int weld_commnext(char *buf, size_t buflen);

struct weld_config weld_config_from_env(void);
int weld_commchk(struct weld_comm *comm);
bool weld_is_same_file(const char *p1, const char *p2);

// calls wordexp(3) on the input line.
// Returns an array of strings.
// The resulting strings are heap allocated and needs to be cleaned up
// using free(2)
char **weld_wordexp(const char *line, size_t *len);
void weld_wordexp_free(char **lines, size_t len);
int weld_mkdirp(const char *path, int mode);

// creates a command from a line
struct weld_comm weld_commfrom(const char *line);

// writes the next token into dst and returns the number of bytes read
// returns -1 on error (e.g. if dst's size is insufficient)
int weld_commtok(char *dst, const char *src, size_t len);

#endif

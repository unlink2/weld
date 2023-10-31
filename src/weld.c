#include "weld.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>
#include <pwd.h>
#include <grp.h>

FILE *weldin = NULL;
FILE *weldout = NULL;
FILE *welderr = NULL;

struct weld_config weldcfg;

void weld_init(struct weld_config cfg) {
  if (weldin == NULL) {
    weldin = stdin;
  }
  if (weldout == NULL) {
    weldout = stdout;
  }
  if (welderr == NULL) {
    welderr = stderr;
  }
  weldcfg = cfg;
}

int weld_main(struct weld_config cfg) {
  weld_init(cfg);

  int ec = 0;

  for (size_t i = 0; i < cfg.argc; i++) {
    if ((weld_commnext(cfg.argv[i], strlen(cfg.argv[i]))) != 0) {
      return -1;
    }
  }

  while ((!isatty(fileno(weldin)) || cfg.argc == 0) &&
         (ec = weld_fcommnext()) == 0) {
  }

  if (ec == 1) {
    ec = 0;
  }

  return ec;
}

bool weld_is_same_file(const char *p1, const char *p2) {
  struct stat s1;
  struct stat s2;
  if (stat(p1, &s1) == -1) {
    fprintf(welderr, "%s: %s\n", p1, strerror(errno));
    return false;
  }
  if (stat(p2, &s2) == -1) {
    fprintf(welderr, "%s: %s\n", p2, strerror(errno));
    return false;
  }

  return s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino;
}

bool weld_confirm(const char *questions, ...) {
  if (!weldcfg.confirm) {
    return true;
  }

  va_list args;
  va_start(args, questions);

  const char *question = questions;
  while (question) {
    fprintf(welderr, "%s", question);
    question = va_arg(args, const char *);
  }
  fputs("(y/n)", welderr);

  int c = getc(weldin);
  return c == 'y';
}

int weld_fmtstat(FILE *f, const char *path) {
  char pbuf[WELD_PATH_MAX];
  memset(pbuf, 0, WELD_PATH_MAX);

  int written = fprintf(f, "%s (", path);
  struct stat st;
  int st_ok = lstat(path, &st);

  // check access if it is not a link
  if ((st_ok == -1 || (st.st_mode & S_IFMT) != S_IFLNK) &&
      access(path, F_OK) == -1) {
    WELD_FMT(f, WELD_CFG_FMT_RED);
    written += fputs("e---------", f);
    WELD_FMT(f, WELD_CFG_FMT_RESET);
    written += fputs(")", f);
    return written;
  }

  char type = '-';

  switch (st.st_mode & S_IFMT) {
  case S_IFDIR:
    type = 'd';
    break;
  case S_IFLNK:
    type = 'l';
    break;
  default:
    type = '-';
    break;
  }

  unsigned int st_mode = st.st_mode;

  WELD_FMT(f, WELD_CFG_FMT_YELLOW);
  fprintf(f, "%c%c%c%c%c%c%c%c%c%c", type, (st_mode & S_IRUSR) ? 'r' : '-',
          (st_mode & S_IWUSR) ? 'w' : '-', (st_mode & S_IXUSR) ? 'x' : '-',
          (st_mode & S_IRGRP) ? 'r' : '-', (st_mode & S_IWGRP) ? 'w' : '-',
          (st_mode & S_IXGRP) ? 'x' : '-', (st_mode & S_IROTH) ? 'r' : '-',
          (st_mode & S_IWOTH) ? 'w' : '-', (st_mode & S_IXOTH) ? 'x' : '-');
  WELD_FMT(f, WELD_CFG_FMT_RESET);

  struct passwd *pws = getpwuid(st.st_uid);
  struct group *grp = getgrgid(st.st_gid);

  // user and group
  WELD_FMT(f, WELD_CFG_FMT_CYAN);
  fprintf(f, " %s %s", pws->pw_name, grp->gr_name);
  WELD_FMT(f, WELD_CFG_FMT_RESET);

  // output link target
  if ((st.st_mode & S_IFMT) == S_IFLNK) {
    size_t len = readlink(path, pbuf, WELD_PATH_MAX);
    if (len == -1) {
      WELD_FMT(f, WELD_CFG_FMT_RED);
      written += fputs("-> ", f);
      WELD_FMT(f, WELD_CFG_FMT_RESET);
    } else {
      if (access(path, F_OK) == 0) {
        WELD_FMT(f, WELD_CFG_FMT_GREEN);
      } else {
        WELD_FMT(f, WELD_CFG_FMT_RED);
      }
      fprintf(f, " -> %s", pbuf);
      WELD_FMT(f, WELD_CFG_FMT_RESET);
    }
  }

  fputs(")", f);

  return written;
}

int weld_commchk(struct weld_comm *comm) {
  if (comm->ok == -1) {
    return -1;
  }

  //  check will only print detailed non-error information
  //  if in a dry run or verbose
  bool display = weldcfg.dry || weldcfg.verbose;

  switch (comm->type) {
  case WELD_COMM_SYMLINK: {
    if (display) {
      WELD_FMT(weldout, WELD_CFG_FMT_GREEN);
      fprintf(weldout, "[create symlink] ");
      WELD_FMT(weldout, WELD_CFG_FMT_RESET);

      weld_fmtstat(weldout, comm->src);
      fputs(" -> ", weldout);

      weld_fmtstat(weldout, comm->dst);
      fputs("\n", weldout);
    }
    // src always has to exists
    if (access(comm->src, F_OK) == -1) {
      WELD_FMT(welderr, WELD_CFG_FMT_RED);
      fprintf(welderr, "'%s': %s\n", comm->src, strerror(errno));
      WELD_FMT(welderr, WELD_CFG_FMT_RESET);
      return -1;
    }

    // dst must not exist if -f is not set
    // but do not error if src and dst are the smae inode
    if (!weldcfg.force && access(comm->dst, F_OK) != -1 &&
        !weld_is_same_file(comm->src, comm->dst)) {
      WELD_FMT(welderr, WELD_CFG_FMT_RED);
      fprintf(welderr, "'%s': File exists\n", comm->dst);
      WELD_FMT(welderr, WELD_CFG_FMT_RESET);
      return -1;
    }
    break;
  }
  case WELD_COMM_NOP:
    break;
  }

  return 0;
}

int weld_commexec(struct weld_comm *comm) {
  switch (comm->type) {
  case WELD_COMM_SYMLINK:
    if (weldcfg.force && access(comm->dst, F_OK) == 0 &&
        weld_confirm(comm->dst, " will be removed! Are you sure? ", NULL)) {
      if (weldcfg.verbose) {
        fprintf(welderr, "rm '%s'\n", comm->dst);
      }
      if (unlink(comm->dst) == -1) {
        WELD_FMT(welderr, WELD_CFG_FMT_RED);
        fprintf(welderr, "'%s': %s\n", comm->dst, strerror(errno));
        WELD_FMT(welderr, WELD_CFG_FMT_RESET);
        return -1;
      }
    }

    if (access(comm->dst, F_OK) == -1) {
      if (weldcfg.verbose) {
        fprintf(welderr, "linking '%s' -> '%s'\n", comm->src, comm->dst);
      }

      if (symlink(comm->src, comm->dst) == -1) {
        WELD_FMT(welderr, WELD_CFG_FMT_RED);
        fprintf(welderr, "'%s' -> '%s': %s\n", comm->src, comm->dst,
                strerror(errno));
        WELD_FMT(welderr, WELD_CFG_FMT_RESET);
        return -1;
      }
    } else if (weld_is_same_file(comm->src, comm->dst)) {
      if (weldcfg.verbose) {
        WELD_FMT(welderr, WELD_CFG_FMT_GREEN);
        fprintf(welderr, "'%s': File exists. Skipped...\n", comm->dst);
        WELD_FMT(welderr, WELD_CFG_FMT_RESET);
      }
      return 0;
    } else {
      WELD_FMT(welderr, WELD_CFG_FMT_RED);
      fprintf(welderr, "'%s': File exists, but is not the expected link\n",
              comm->dst);
      WELD_FMT(welderr, WELD_CFG_FMT_RESET);
      return -1;
    }
    break;
  default:
    break;
  }
  return 0;
}

int weld_commdo(const char *line) {
  struct weld_comm c = weld_commfrom(line);
  int ok = weld_commchk(&c);
  if (ok == -1) {
    return -1;
  }

  // do not proceed in a dry run
  if (weldcfg.dry) {
    return 0;
  }

  return weld_commexec(&c);
}

// next command from in file
int weld_fcommnext(void) {
  // read a line from in
  char buf[WELD_BUF_MAX];
  memset(buf, 0, WELD_BUF_MAX);

  if (fgets(buf, WELD_BUF_MAX, weldin) == 0) {
    return 1;
  }
  buf[strcspn(buf, "\n")] = 0;

  return weld_commnext(buf, WELD_BUF_MAX);
}

int weld_commnext(char *buf, size_t buflen) {
  // handle comments before wordexpand to prevent expanding lines starting with
  // #
  for (size_t i = 0;
       i < buflen && (isspace(buf[i]) || buf[i] == WELD_COMM_COMMENT); i++) {
    if (buf[i] == WELD_COMM_COMMENT) {
      return 0;
    }
  }

  if (weldcfg.expand) {
    size_t len = 0;
    char **expanded = weld_wordexp(buf, &len);

    if (expanded == NULL) {
      return -1;
    }

    for (size_t i = 0; i < len; i++) {
      if (weld_commdo(expanded[i]) == -1) {
        weld_wordexp_free(expanded, len);
        return -1;
      }
    }

    weld_wordexp_free(expanded, len);

    return 0;
  }

  return weld_commdo(buf);
}

struct weld_config weld_config_from_env(void) {
  struct weld_config cfg;
  memset(&cfg, 0, sizeof(cfg));

  cfg.verbose = getenv(WELD_VERBOSE) != NULL;
  cfg.color = true;
  cfg.mkdir_mode = 0777;

  return cfg;
}

const char *weld_worderr(int worderr) {
  switch (worderr) {
  case WRDE_BADCHAR:
    return "Unquoted character";
  case WRDE_BADVAL:
    return "Undefined shell variable was referenced";
  case WRDE_CMDSUB:
    return "Command substitution used when WRDE_NOCMD was set";
  case WRDE_NOSPACE:
    return "Malloc failed";
  case WRDE_SYNTAX:
    return "Syntax error";
  default:
    return "Unkonw error";
  }

  return "";
}

char **weld_wordexp(const char *line, size_t *len) {
  wordexp_t p;
  char **w = NULL;

  int worderr = wordexp(line, &p, WRDE_NOCMD);
  if (worderr != 0) {
    fprintf(welderr, "wordexp failed: %s\n", weld_worderr(worderr));
    goto FAIL;
  }

  w = malloc(sizeof(char *) * p.we_wordc);
  *len = p.we_wordc;

  for (size_t i = 0; i < p.we_wordc; i++) {
    w[i] = strdup(p.we_wordv[i]);
  }
FAIL:
  wordfree(&p);
  return w;
}

void weld_wordexp_free(char **lines, size_t len) {
  for (size_t i = 0; i < len; i++) {
    free(lines[i]);
  }

  free(lines);
}

int weld_commtok(char *dst, const char *src, size_t len) {
  if (weldcfg.verbose) {
    fprintf(welderr, "[commtok] Parsing '%s'. max len %ld\n", src, len);
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
    fprintf(welderr,
            "[commtok] Result: '%s'. %d bytes read. Reamining source: '%s'\n",
            dst_start, i, src);
  }

  if (*src && *src != WELD_COMM_TERM) {
    WELD_FMT(welderr, WELD_CFG_FMT_RED);
    fprintf(welderr, "The supplied buffer did not provide enough memory to fit "
                     "the entire token!\n");
    WELD_FMT(welderr, WELD_CFG_FMT_RESET);
    return -1;
  }

  // advance past the separator
  if (*src == WELD_COMM_TERM) {
    i++;
  }

  return i;
}

struct weld_comm weld_commfrom(const char *line) {
  const char *line_start = line;

  int read = 0;
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
      fprintf(welderr, "[commfrom] read symlink with src: '%s' and dst: '%s'\n",
              comm.src, comm.dst);
    }
    break;
  default:
    WELD_FMT(welderr, WELD_CFG_FMT_RED);
    fprintf(welderr, "Unknown command type: '%c'\n", typebuf[0]);
    WELD_FMT(welderr, WELD_CFG_FMT_RESET);
    goto FAIL;
  }

SKIP_COMMENT:
  comm.ok = 0;
  return comm;
FAIL:
  WELD_FMT(welderr, WELD_CFG_FMT_RED);
  fprintf(welderr, "Parsing command '%s' failed\n", line_start);
  WELD_FMT(welderr, WELD_CFG_FMT_RESET);
  return comm;
}

#include "weld.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
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

struct weld_stat weld_stat(const char *path) {
  struct weld_stat wstat;
  memset(&wstat, 0, sizeof(wstat));
  wstat.ok = 0;
  wstat.exists = true;
  wstat.path = path;
  struct stat fs;

  if (lstat(path, &fs) < 0) {
    goto FAIL;
  }

  wstat.st = fs;
  return wstat;
FAIL:
  // fprintf(welderr, "%s: %s\n", path, strerror(errno));
  wstat.exists = false;
  return wstat;
}

size_t weld_fmtstat(FILE *f, struct weld_stat *wstat) {
  if (wstat->ok == -1) {
    return 0;
  }

  char pbuf[WELD_PATH_MAX];
  memset(pbuf, 0, WELD_PATH_MAX);

  size_t written = fprintf(f, "%s (", wstat->path);
  if (!wstat->exists) {
    WELD_FMT(f, WELD_CFG_FMT_RED);
    written += fputs("E", f);
    WELD_FMT(f, WELD_CFG_FMT_RESET);
    written += fputs(")", f);
    return written;
  }

  WELD_FMT(f, WELD_CFG_FMT_YELLOW);
  fprintf(f, "%o", wstat->st.st_mode);
  WELD_FMT(f, WELD_CFG_FMT_RESET);

  struct passwd *pws = getpwuid(wstat->st.st_uid);
  struct group *grp = getgrgid(wstat->st.st_gid);

  WELD_FMT(f, WELD_CFG_FMT_CYAN);
  fprintf(f, " %s %s", pws->pw_name, grp->gr_name);
  WELD_FMT(f, WELD_CFG_FMT_RESET);

  // TODO: also check access() here
  if ((wstat->st.st_mode & S_IFMT) == S_IFLNK) {
    size_t len = readlink(wstat->path, pbuf, WELD_PATH_MAX);
    if (len == -1) {
      WELD_FMT(f, WELD_CFG_FMT_RED);
      written += fputs("-> E", f);
      WELD_FMT(f, WELD_CFG_FMT_RESET);
    } else {
      if (access(wstat->path, F_OK) == 0) {
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

struct weld_commchk weld_commchk(struct weld_comm *comm) {
  struct weld_commchk chk;
  memset(&chk, 0, sizeof(chk));
  chk.ok = -1;
  chk.comm = comm;

  if (comm->ok == -1) {
    goto FAIL;
  }

  //  check will only print detailed non-error information
  //  if in a dry run or verbose
  bool display = weldcfg.dry || weldcfg.verbose;

  switch (comm->type) {
  case WELD_COMM_SYMLINK:
    chk.src_stat = weld_stat(comm->src);
    chk.dst_stat = weld_stat(comm->dst);
    if (display) {
      WELD_FMT(weldout, WELD_CFG_FMT_GREEN);
      fprintf(weldout, "[create symlink] ");
      WELD_FMT(weldout, WELD_CFG_FMT_RESET);

      weld_fmtstat(weldout, &chk.src_stat);
      fputs(" -> ", weldout);

      weld_fmtstat(weldout, &chk.dst_stat);
      fputs("\n", weldout);
    }

    if (chk.src_stat.ok == -1) {
      goto FAIL;
    }
    break;
  case WELD_COMM_NOP:
    break;
  }

  chk.ok = 0;
FAIL:
  return chk;
}

int weld_commdo(const char *line) {
  struct weld_comm c = weld_commfrom(line);
  struct weld_commchk chk = weld_commchk(&c);
  if (chk.ok == -1) {
    return -1;
  }

  // do not proceed in a dry run
  if (weldcfg.dry) {
    return 0;
  }

  return 0;
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

  cfg.color = true;

  return cfg;
}

char **weld_wordexp(const char *line, size_t *len) {
  wordexp_t p;
  char **w = NULL;

  if (wordexp(line, &p, WRDE_NOCMD) != 0) {
    fprintf(welderr, "wprdexp failed\n");
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
    fprintf(welderr, "The supplied buffer did not provide enough memory to fit "
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
    fprintf(welderr, "Unknown command type: '%c'\n", typebuf[0]);
    goto FAIL;
  }

SKIP_COMMENT:
  comm.ok = 0;
  return comm;
FAIL:
  fprintf(welderr, "Parsing command '%s' failed\n", line_start);
  return comm;
}

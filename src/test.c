#include "weld.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WELD_TMPD_TEMPLATE "weld-test-XXXXXX"
#define WELD_TMPDIR "WELD_TMPDIR"

#define assert_commpath(expect_str, expect_read, src, buf, len)                \
  memset(buf, 0, len);                                                         \
  assert(weld_commtok((buf), (src), (len)) == (expect_read));                  \
  assert(strcmp((expect_str), (buf)) == 0);

void test_commpath(void) {
  puts("[commpath test]");

  const int len = 128;
  char buf[len];

  assert_commpath("test", 4, "test", buf, len);
  assert_commpath("tes:t", 6, "tes\\:t", buf, len);

  // this one should consume 5 chars to advance past the separator!
  assert_commpath("test", 5, "test:123", buf, len);

  assert_commpath("", 0, "", buf, len);

  assert_commpath("a", 1, "a", buf, 2);
  assert_commpath("a", 2, "a:", buf, 2);
  assert_commpath("a", 2, "a:b", buf, 2);

  // error case: buffer cannot fit entire token
  assert_commpath("tes", -1, "test:123", buf, 4);

  puts("[commpath ok]");
}

#define assert_commfrom_sl(expect_src, expect_dst, expect_ok, line)            \
  {                                                                            \
    struct weld_comm c = weld_commfrom((line));                                \
    assert((expect_ok) == c.ok);                                               \
    assert((WELD_COMM_SYMLINK) == c.type);                                     \
    assert(strcmp((expect_src), c.src) == 0);                                  \
    assert(strcmp((expect_dst), c.dst) == 0);                                  \
  }

#define assert_commfrom_nop(line, expect_ok)                                   \
  {                                                                            \
    struct weld_comm c = weld_commfrom((line));                                \
    assert((expect_ok) == c.ok);                                               \
    assert((WELD_COMM_NOP) == c.type);                                         \
  }

#define assert_commfrom_inval(line, expect_ok)                                 \
  {                                                                            \
    struct weld_comm c = weld_commfrom((line));                                \
    assert((expect_ok) == c.ok);                                               \
  }

void test_commfrom(void) {
  puts("[commfrom test]");

  assert_commfrom_sl("source/path", "dest/path", 0, "s:source/path:dest/path");
  assert_commfrom_sl("source/path", "", -1, "s:source/path");
  assert_commfrom_sl("", "", -1, "s");

  assert_commfrom_nop("# comment", 0);
  assert_commfrom_nop("   # comment", 0);

  assert_commfrom_inval("", -1);
  assert_commfrom_inval("-", -1);
  assert_commfrom_inval("---", -1);
  assert_commfrom_inval("sl", -1);
  assert_commfrom_inval("sll", -1);

  puts("[commfrom ok]");
}

#define assert_wordexp(expectv0, expect_len, line)                             \
  {                                                                            \
    size_t len = 0;                                                            \
    char **res = weld_wordexp((line), &len);                                   \
    assert(len == (expect_len));                                               \
    assert(strcmp(expectv0, res[0]) == 0);                                     \
    weld_wordexp_free(res, len);                                               \
  }

void test_wordexp(void) {
  puts("[wordexp test]");

  setenv("WELD_TEST", "123", 0);
  assert_wordexp("s:/path/123:/path2/123", 1,
                 "s:/path/$WELD_TEST:/path2/$WELD_TEST");
  assert_wordexp("s:/path/123:/path2/123", 1, "s:/path/123:/path2/123");

  puts("[wordexp ok]");
}

#define assert_dry(expect, expect_ret, input)                                  \
  {                                                                            \
    weldcfg.argv = (char *[]){(input)};                                        \
    weldcfg.argc = 1;                                                          \
    char buf[4096];                                                            \
    memset(buf, 0, 4096);                                                      \
    FILE *f = fmemopen(buf, 4096, "w");                                        \
    weldout = f;                                                               \
    assert(weld_main(weldcfg) == (expect_ret));                                \
    fclose(f);                                                                 \
    size_t len = 0;                                                            \
    char **expanded = weld_wordexp(expect, &len);                              \
    assert(expanded);                                                          \
    assert(len > 0);                                                           \
    printf("%s", buf);                                      \
    assert(strcmp(expanded[0], buf) == 0);                                     \
    weldout = stdout;                                                          \
    weld_wordexp_free(expanded, len);                                          \
  }

// test the output of a dry run here
// including reading fake data
// from a directory specifically made to
// test the program
void test_dry(void) {
  weldcfg.dry = true;

  puts("[dry test]");

  assert_dry("\"[create symlink] ./f.weld (e---------) -> ./f-link.weld "
             "(e---------)\n\"",
             -1, "s:./f.weld:./f-link.weld");

  assert_dry("\"[create symlink] ./f0.weld (-r--r--r-- $USER $USER) -> "
             "./f0-link.wedl (e---------)\n\"",
             0, "s:./f0.weld:./f0-link.wedl");

  assert_dry("\"[create symlink] ./f2.weld (-r--r--r-- $USER $USER) -> "
             "./f2-link.weld (lrwxrwxrwx $USER $USER -> f.weld)\n\"",
             0, "s:./f2.weld:./f2-link.weld");

  assert_dry("\"[create symlink] ./f3.weld (-r--r--r-- $USER $USER) -> "
             "./f3-link.weld (lrwxrwxrwx $USER $USER -> f3.weld)\n\"",
             0, "s:./f3.weld:./f3-link.weld");

  assert_dry("\"[create symlink] ./f4.weld (-r--r--r-- $USER $USER) -> "
             "./f4-link.weld (-r--r--r-- $USER $USER)\n\"",
             0, "s:./f4.weld:./f4-link.weld");

  puts("[dry ok]");
}

void test_is_same_file(void) {
  puts("[same file test]");

  assert(weld_is_same_file("./f3.weld", "./f3-link.weld"));
  assert(!weld_is_same_file("./f4.weld", "./f4-link.weld"));
  assert(!weld_is_same_file("./f2.weld", "./f2-link.weld"));

  puts("[same file ok]");
}

// simplt creates a new file
int weld_touch(const char *path) {
  int fd =
      open(path, O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IRGRP | S_IROTH);
  assert(fd != -1);
  return fd;
}

/**
 * This function sets up a directory that is to be used by the tests
 * the directory shall contain some dummy files that the tests can operate on
 * the tests will chdir into the respective test directoy
 * This test will place the directory in the directory specified in WELD_TMPDIR
 * or /tmp
 * All files that are used by the tests end in .weld.
 * This is simply to ensure we do not accidentally modify or delete files in the
 * tmp directory that might have been useful.
 */
void weld_test_init(void) {
  char *path = getenv(WELD_TMPDIR);
  if (!path) {
    path = "./tmp";
  }

  assert(path);
  assert(chdir(path) == 0);

  // set up files that tests expect to exist here
  weld_touch("f0.weld");
  weld_touch("f1.weld");
  weld_touch("f2.weld");
  weld_touch("f3.weld");
  weld_touch("f4.weld");

  // file instead of link
  weld_touch("f4-link.weld");

  // valid link
  assert(symlink("f3.weld", "f3-link.weld") != -1);

  // link that points nowhere
  assert(symlink("f.weld", "f2-link.weld") != -1);

  printf("[test location '%s']\n", path);
}

int main(int arc, char **argv) {
  weld_init(weld_config_from_env());
  weldcfg.verbose = false;

  weld_test_init();

  puts("[tests]");

  test_commpath();
  test_commfrom();
  test_wordexp();
  test_dry();
  test_is_same_file();

  puts("[tests ok]");
  return 0;
}

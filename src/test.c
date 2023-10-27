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

// test the output of a dry run here
// including reading fake data
// from a directory specifically made to
// test the program
void test_dry(void) {}

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

  puts("[wordexp ok]");
}

/**
 * This function sets up a directory that is to be used by the tests
 * the directory shall contain some dummy files that the tests can operate on
 * the tests will chdir into the respective test directoy
 * This test will place the directory in the directory specified in WELD_TMPDIR
 * or /tmp
 */
void weld_test_init(void) {
  char *path = getenv(WELD_TMPDIR);
  if (!path) {
    path = "./tmp";
  }

  assert(path);
  assert(chdir(path) == 0);
  printf("[tests '%s']\n", path);
}

int main(int arc, char **argv) {
  weld_init(weld_config_from_env());
  weldcfg.verbose = false;

  weld_test_init();

  puts("[tests]");

  test_commpath();
  test_commfrom();
  test_wordexp();

  puts("[tests ok]");
  return 0;
}

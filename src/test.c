#include "weld.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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
  }

void test_commfrom(void) {
  puts("[commfrom test]");

  assert_commfrom_sl("", "", 0, "s:source/path:dest/path");

  puts("[commfrom ok]");
}

int main(int arc, char **argv) {
  weldcfg.verbose = true;

  test_commpath();
  test_commfrom();

  return 0;
}

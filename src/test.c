#include "weld.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define assert_commpath(expect_str, expect_read, src, buf, len)                \
  assert(weld_commtok((buf), (src), (len)) == (expect_read));                  \
  assert(strcmp((expect_str), (buf)) == 0);

void test_commpath(void) {
  puts("[commpath test]");

  const int len = 128;
  char buf[len];

  assert_commpath("test", 4, "test", buf, len);
  assert_commpath("tes:t", 6, "tes\\:t", buf, len);
  assert_commpath("test", 4, "test:123", buf, len);

  // error case: buffer cannot fit entire token
  assert_commpath("tes", -1, "test:123", buf, 4);

  puts("[commpath ok]");
}

int main(int arc, char **argv) {
  test_commpath();

  return 0;
}

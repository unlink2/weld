#include "weld.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define assert_commpath(expect, src, buf, len)                                 \
  assert(strcmp((expect), weld_commpath((buf), (src), (len))) == 0);

void test_commpath(void) {
  puts("[commpath test]");

  const int len = 128;
  char buf[len];

  assert_commpath("test", "test", buf, len);
  assert_commpath("tes:t", "tes\\:t", buf, len);
  assert_commpath("test", "test:123", buf, len);

  puts("[commpath ok]");
}

int main(int arc, char **argv) {
  test_commpath();

  return 0;
}

#include "weld.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

void test_commpath(void **state) {
  const int len = 128;
  char buf[len];

  assert_string_equal("test", weld_commpath(buf, "test", len));
}

int main(int arc, char **argv) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_commpath),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}

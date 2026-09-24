#include <cstdlib>
#include <gtest/gtest.h>
#include "stack_test.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  int ret = RUN_ALL_TESTS();
#ifdef _WIN32
  system("pause");
#endif

  return ret;
}

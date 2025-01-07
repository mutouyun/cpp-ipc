#include <iostream>

#include "test.h"

#include "libipc/imp/error.h"
#include "libipc/imp/fmt.h"

TEST(error, error_code) {
  std::error_code ecode;
  EXPECT_FALSE(ecode);
  std::cout << ipc::fmt(ecode, '\n');
}

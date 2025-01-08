
#include "test.h"

#include "libipc/imp/system.h"

TEST(system, conf) {
  auto ret = ipc::sys::conf(ipc::sys::info::page_size);
  EXPECT_TRUE(ret);
  EXPECT_EQ(ret.value(), 4096);
}

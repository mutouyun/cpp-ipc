
#include "gtest/gtest.h"

#include <libipc/event.h>

TEST(event, open_close) {
  auto evt = ipc::evt_open("test");
  ASSERT_TRUE(evt);
  ASSERT_EQ(ipc::evt_name(*evt), "test");
  ASSERT_TRUE(ipc::evt_close(*evt));
}

TEST(event, set_wait) {
  auto evt = ipc::evt_open("test");
  ASSERT_TRUE(evt);
  ASSERT_TRUE(ipc::evt_set(*evt));
  ASSERT_TRUE(ipc::evt_wait(*evt, 0));
  ASSERT_TRUE(ipc::evt_close(*evt));
}

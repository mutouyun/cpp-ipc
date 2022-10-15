
#include "gtest/gtest.h"

#include "libipc/shm.h"

TEST(shm, create_close) {
  EXPECT_FALSE(ipc::shm_open("hello-ipc-shm", 1024, ipc::mode::none));

  auto shm1 = ipc::shm_open("hello-ipc-shm", 1024, ipc::mode::create | ipc::mode::open);
  EXPECT_TRUE(shm1);
  EXPECT_FALSE(ipc::shm_open("hello-ipc-shm", 1024, ipc::mode::create));

  auto pt1 = ipc::shm_get(*shm1);
  EXPECT_TRUE(ipc::shm_size(*shm1) >= 1024);
  EXPECT_NE(pt1, nullptr);
  *(int *)pt1 = 0;

  auto shm2 = ipc::shm_open(ipc::shm_file(*shm1), 0, ipc::mode::open);
  EXPECT_TRUE(shm2);
  EXPECT_EQ(ipc::shm_size(*shm1), ipc::shm_size(*shm2));
  auto pt2 = ipc::shm_get(*shm1);
  EXPECT_NE(pt2, nullptr);
  EXPECT_EQ(*(int *)pt2, 0);
  *(int *)pt1 = 1234;
  EXPECT_EQ(*(int *)pt2, 1234);

  EXPECT_TRUE(ipc::shm_close(*shm2));
  EXPECT_TRUE(ipc::shm_close(*shm1));
  EXPECT_FALSE(ipc::shm_close(nullptr));
}
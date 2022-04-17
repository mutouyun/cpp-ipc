
#include <sstream>

#include "gtest/gtest.h"

#include "libipc/result.h"

TEST(result, ok) {
  ipc::result ret;
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.code(), 0);

  ret = {true};
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.code(), 0);

  ret = {false, 1234};
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.code(), 1234);
}

TEST(result, code) {
  ipc::result ret(true, 1234);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.code(), 1234);

  ret = {false};
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.code(), 0);

  ret = {true, 4321};
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.code(), 4321);
}

TEST(result, compare) {
  ipc::result r1, r2;
  EXPECT_EQ(r1, r2);

  ipc::result r3(true);
  EXPECT_NE(r1, r3);

  ipc::result r4(true, 222222);
  EXPECT_NE(r3, r4);

  ipc::result r5(false, 222222);
  EXPECT_NE(r4, r5);
  r3 = r5;
  EXPECT_EQ(r3, r5);
}

TEST(result, print) {
  std::stringstream ss;
  ipc::result r1;
  ss << r1;
  EXPECT_EQ(ss.str(), "[fail, code = 0]");

  ss = {};
  ipc::result r2(true, 65537);
  ss << r2;
  EXPECT_EQ(ss.str(), "[succ, code = 65537]");

  ss = {};
  ipc::result r3(true);
  ss << r3;
  EXPECT_EQ(ss.str(), "[succ, code = 0]");
}
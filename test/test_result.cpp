
#include <sstream>

#include "gtest/gtest.h"
#include "fmt/format.h"

#include "libimp/result.h"

TEST(result, ok) {
  imp::result_code ret;
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = {true};
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = {false, 1234};
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(*ret, 1234);
}

TEST(result, code) {
  imp::result_code ret(true, 1234);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 1234);

  ret = {false};
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = {true, 4321};
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 4321);
}

TEST(result, compare) {
  imp::result_code r1, r2;
  EXPECT_EQ(r1, r2);

  imp::result_code r3(true);
  EXPECT_NE(r1, r3);

  imp::result_code r4(true, 222222);
  EXPECT_NE(r3, r4);

  imp::result_code r5(false, 222222);
  EXPECT_NE(r4, r5);
  r3 = r5;
  EXPECT_EQ(r3, r5);
}

TEST(result, fmt) {
  {
    imp::result_code r1;
    EXPECT_EQ(fmt::format("{}", r1), "[fail, value = 0]");
    imp::result_code r2(true, 65537);
    EXPECT_EQ(fmt::format("{}", r2), "[succ, value = 65537]");
    imp::result_code r3(true);
    EXPECT_EQ(fmt::format("{}", r3), "[succ, value = 0]");
  }
  {
    imp::result<int> r1 {false, -123};
    EXPECT_EQ(fmt::format("{}", r1), fmt::format("[fail, value = {}]", -123));
    imp::result<void *> r2 {true, &r1};
    EXPECT_EQ(fmt::format("{}", r2), fmt::format("[succ, value = {}]", (void *)&r1));
    int aaa {};
    imp::result<int *> r3 {false, &aaa};
    EXPECT_EQ(fmt::format("{}", r3), fmt::format("[fail, value = {}]", (void *)&aaa));
  }
}
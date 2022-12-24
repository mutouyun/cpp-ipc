
#include <sstream>
#include <cstdint>

#include "gtest/gtest.h"

#include "libimp/result.h"
#include "libimp/fmt.h"

TEST(result, ok) {
  imp::result_code ret;
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = {true, 0};
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = {false, 1234};
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(*ret, 1234);

  imp::result<int *> r2 {nullptr, 4321};
  EXPECT_NE(r2, nullptr); // imp::result<int *>{nullptr}
  EXPECT_EQ(*r2, nullptr);
  EXPECT_FALSE(r2);
}

TEST(result, code) {
  imp::result_code ret(true, 1234);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 1234);

  ret = {false, 0};
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
    EXPECT_EQ(imp::fmt(r1), "[fail, value = 0]");
    imp::result_code r2(true, 65537);
    EXPECT_EQ(imp::fmt(r2), "[succ, value = 65537]");
    imp::result_code r3(0);
    EXPECT_EQ(imp::fmt(r3), "[succ, value = 0]");
  }
  {
    imp::result<int> r0;
    EXPECT_EQ(imp::fmt(r0), imp::fmt(imp::result_code()));
    imp::result<int> r1 {false, -123};
    EXPECT_EQ(imp::fmt(r1), imp::fmt("[fail, value = ", -123, "]"));

    imp::result<void *> r2 {&r1};
    EXPECT_EQ(imp::fmt(r2), imp::fmt("[succ, value = ", (void *)&r1, "]"));

    int aaa {};
    imp::result<int *> r3 {&aaa};
    EXPECT_EQ(imp::fmt(r3), imp::fmt("[succ, value = ", (void *)&aaa, "]"));
    imp::result<int *> r4 {nullptr};
    EXPECT_EQ(imp::fmt(r4), imp::fmt("[fail, value = ", nullptr, ", code = 0]"));
    r4 = {nullptr, 1234};
    EXPECT_EQ(imp::fmt(r4), imp::fmt("[fail, value = ", nullptr, ", code = 1234]"));
    imp::result<int *> r5;
    EXPECT_EQ(imp::fmt(r5), "[fail, value = null, code = 0]");
  }
  {
    imp::result<std::int64_t> r1 {-123};
    EXPECT_EQ(imp::fmt(r1), imp::fmt("[succ, value = ", -123, "]"));
  }
}
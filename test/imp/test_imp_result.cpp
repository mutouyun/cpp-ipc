
#include <sstream>
#include <cstdint>

#include "test.h"

#include "libipc/imp/result.h"

TEST(result, ok) {
  ipc::result<std::uint64_t> ret;
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = {0};
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = ipc::result<std::uint64_t>(1234);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 1234);

  ret = std::error_code{9999, std::generic_category()};
  EXPECT_FALSE(ret);
  EXPECT_FALSE(ret.ok());
  EXPECT_EQ(ret.value(), 0);

  ret = 4321;
  EXPECT_TRUE(ret);
  EXPECT_TRUE(ret.ok());
  EXPECT_EQ(ret.value(), 4321);

  ipc::result<void> r1;
  EXPECT_FALSE(r1);
  r1 = std::error_code{};
  EXPECT_TRUE(r1);
  r1 = {};
  EXPECT_FALSE(r1);
  r1 = std::error_code{9999, std::generic_category()};
  EXPECT_FALSE(r1);
  EXPECT_EQ(r1.error().value(), 9999);

  ipc::result<int *> r2 {nullptr, std::error_code{4321, std::generic_category()}};
  EXPECT_NE(r2, nullptr); // ipc::result<int *>{nullptr}
  EXPECT_EQ(*r2, nullptr);
  EXPECT_FALSE(r2);
}

TEST(result, compare) {
  ipc::result<std::uint64_t> r1, r2;
  EXPECT_EQ(r1, r2);

  ipc::result<std::uint64_t> r3(0);
  EXPECT_NE(r1, r3);

  ipc::result<std::uint64_t> r4(222222);
  EXPECT_NE(r3, r4);

  ipc::result<std::uint64_t> r5(std::error_code{9999, std::generic_category()});
  EXPECT_NE(r4, r5);
  EXPECT_NE(r3, r5);

  r3 = r5;
  EXPECT_EQ(r3, r5);
}

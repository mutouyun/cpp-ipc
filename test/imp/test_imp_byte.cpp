
#include "gtest/gtest.h"

#include "libimp/byte.h"
#include "libimp/span.h"
#include "libimp/fmt.h"
#include "libimp/detect_plat.h"

TEST(byte, construct) {
  {
    LIBIMP_UNUSED imp::byte b;
    SUCCEED();
  }
  {
    imp::byte b{};
    EXPECT_EQ(int(b), 0);
  }
  {
    imp::byte b{123};
    EXPECT_EQ(int(b), 123);
  }
  {
    imp::byte b{65535};
    EXPECT_EQ(int(b), 255);
    EXPECT_EQ(std::int8_t(b), -1);
  }
  {
    imp::byte b{65536};
    EXPECT_EQ(int(b), 0);
  }
}

TEST(byte, compare) {
  {
    imp::byte b1{}, b2{};
    EXPECT_EQ(b1, b2);
  }
  {
    imp::byte b1{}, b2(321);
    EXPECT_NE(b1, b2);
  }
}

TEST(byte, fmt) {
  {
    imp::byte b1{}, b2(31);
    EXPECT_EQ(imp::fmt(b1), "00");
    EXPECT_EQ(imp::fmt(b2), "1f");
    EXPECT_EQ(imp::fmt(imp::spec("03X")(b2)), "01F");
  }
  {
    imp::byte bs[] {31, 32, 33, 34, 35, 36, 37, 38};
    EXPECT_EQ(imp::fmt(imp::make_span(bs)), "1f 20 21 22 23 24 25 26");
  }
}

TEST(byte, byte_cast) {
  int a = 654321;
  int *pa = &a;

  // int * => byte *
  imp::byte *pb = imp::byte_cast(pa);
  EXPECT_EQ((std::size_t)pb, (std::size_t)pa);

  // byte * => int32_t *
  std::int32_t *pc = imp::byte_cast<std::int32_t>(pb);
  EXPECT_EQ(*pc, a);

  // byte alignment check
  EXPECT_EQ(imp::byte_cast<int>(pb + 1), nullptr);
}
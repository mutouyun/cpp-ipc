
#include "test.h"

#include "libipc/imp/byte.h"
#include "libipc/imp/span.h"
#include "libipc/imp/detect_plat.h"

TEST(byte, construct) {
  {
    LIBIPC_UNUSED ipc::byte b;
    SUCCEED();
  }
  {
    ipc::byte b{};
    EXPECT_EQ(int(b), 0);
  }
  {
    ipc::byte b{123};
    EXPECT_EQ(int(b), 123);
  }
  {
    ipc::byte b{65535};
    EXPECT_EQ(int(b), 255);
    EXPECT_EQ(std::int8_t(b), -1);
  }
  {
    ipc::byte b{65536};
    EXPECT_EQ(int(b), 0);
  }
}

TEST(byte, compare) {
  {
    ipc::byte b1{}, b2{};
    EXPECT_EQ(b1, b2);
  }
  {
    ipc::byte b1{}, b2(321);
    EXPECT_NE(b1, b2);
  }
}

TEST(byte, byte_cast) {
  int a = 654321;
  int *pa = &a;

  // int * => byte *
  ipc::byte *pb = ipc::byte_cast(pa);
  EXPECT_EQ((std::size_t)pb, (std::size_t)pa);

  // byte * => int32_t *
  std::int32_t *pc = ipc::byte_cast<std::int32_t>(pb);
  EXPECT_EQ(*pc, a);

  // byte alignment check
  EXPECT_EQ(ipc::byte_cast<int>(pb + 1), nullptr);
}

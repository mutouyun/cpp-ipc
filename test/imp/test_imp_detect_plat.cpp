
#include <iostream>
#include <cstdint>

#include "gtest/gtest.h"

#include "libimp/detect_plat.h"

namespace {

} // namespace

TEST(detect_plat, os) {
#if defined(LIBIMP_OS_WINCE)
  std::cout << "LIBIMP_OS_WINCE\n";
#elif defined(LIBIMP_OS_WIN)
  std::cout << "LIBIMP_OS_WIN\n";
#elif defined(LIBIMP_OS_LINUX)
  std::cout << "LIBIMP_OS_LINUX\n";
#elif defined(LIBIMP_OS_QNX)
  std::cout << "LIBIMP_OS_QNX\n";
#elif defined(LIBIMP_OS_ANDROID)
  std::cout << "LIBIMP_OS_ANDROID\n";
#else
  ASSERT_TRUE(false);
#endif
  SUCCEED();
}

TEST(detect_plat, cc) {
#if defined(LIBIMP_CC_MSVC)
  std::cout << "LIBIMP_CC_MSVC\n";
#elif defined(LIBIMP_CC_GNUC)
  std::cout << "LIBIMP_CC_GNUC\n";
#else
  ASSERT_TRUE(false);
#endif
  SUCCEED();
}

TEST(detect_plat, cpp) {
#if defined(LIBIMP_CPP_20)
  std::cout << "LIBIMP_CPP_20\n";
#elif defined(LIBIMP_CPP_17)
  std::cout << "LIBIMP_CPP_17\n";
#elif defined(LIBIMP_CPP_14)
  std::cout << "LIBIMP_CPP_14\n";
#else
  ASSERT_TRUE(false);
#endif
  SUCCEED();
}

TEST(detect_plat, byte_order) {
  auto is_endian_little = [] {
    union {
      std::int32_t a;
      std::int8_t  b;
    } c;
    c.a = 1;
    return c.b == 1;
  };
  EXPECT_EQ(!!LIBIMP_ENDIAN_LIT, is_endian_little());
  EXPECT_NE(!!LIBIMP_ENDIAN_BIG, is_endian_little());
}

TEST(detect_plat, fallthrough) {
  switch (0) {
  case 0:
    std::cout << "fallthrough 0\n";
    LIBIMP_FALLTHROUGH;
  case 1:
    std::cout << "fallthrough 1\n";
    LIBIMP_FALLTHROUGH;
  default:
    std::cout << "fallthrough default\n";
    break;
  }
  SUCCEED();
}

TEST(detect_plat, unused) {
  LIBIMP_UNUSED int abc;
  SUCCEED();
}

TEST(detect_plat, likely_unlikely) {
  int xx = sizeof(int);
  if LIBIMP_LIKELY(xx < sizeof(long long)) {
    std::cout << "sizeof(int) < sizeof(long long)\n";
  } else if LIBIMP_UNLIKELY(xx < sizeof(char)) {
    std::cout << "sizeof(int) < sizeof(char)\n";
  } else {
    std::cout << "sizeof(int) < whatever\n";
  }
  SUCCEED();
}
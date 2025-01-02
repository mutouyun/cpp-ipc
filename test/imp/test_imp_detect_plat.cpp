
#include "test.h"

#include "libipc/imp/detect_plat.h"

TEST(detect_plat, os) {
#if defined(LIBIPC_OS_WINCE)
  std::cout << "LIBIPC_OS_WINCE\n";
#elif defined(LIBIPC_OS_WIN)
  std::cout << "LIBIPC_OS_WIN\n";
#elif defined(LIBIPC_OS_LINUX)
  std::cout << "LIBIPC_OS_LINUX\n";
#elif defined(LIBIPC_OS_QNX)
  std::cout << "LIBIPC_OS_QNX\n";
#elif defined(LIBIPC_OS_ANDROID)
  std::cout << "LIBIPC_OS_ANDROID\n";
#else
  ASSERT_TRUE(false);
#endif
  SUCCEED();
}

TEST(detect_plat, cc) {
#if defined(LIBIPC_CC_MSVC)
  std::cout << "LIBIPC_CC_MSVC\n";
#elif defined(LIBIPC_CC_GNUC)
  std::cout << "LIBIPC_CC_GNUC\n";
#else
  ASSERT_TRUE(false);
#endif
  SUCCEED();
}

TEST(detect_plat, cpp) {
#if defined(LIBIPC_CPP_20)
  std::cout << "LIBIPC_CPP_20\n";
#elif defined(LIBIPC_CPP_17)
  std::cout << "LIBIPC_CPP_17\n";
#elif defined(LIBIPC_CPP_14)
  std::cout << "LIBIPC_CPP_14\n";
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
  EXPECT_EQ(!!LIBIPC_ENDIAN_LIT, is_endian_little());
  EXPECT_NE(!!LIBIPC_ENDIAN_BIG, is_endian_little());
}

TEST(detect_plat, fallthrough) {
  switch (0) {
  case 0:
    std::cout << "fallthrough 0\n";
    LIBIPC_FALLTHROUGH;
  case 1:
    std::cout << "fallthrough 1\n";
    LIBIPC_FALLTHROUGH;
  default:
    std::cout << "fallthrough default\n";
    break;
  }
  SUCCEED();
}

TEST(detect_plat, unused) {
  LIBIPC_UNUSED int abc;
  SUCCEED();
}

TEST(detect_plat, likely_unlikely) {
  int xx = sizeof(int);
  if LIBIPC_LIKELY(xx < sizeof(long long)) {
    std::cout << "sizeof(int) < sizeof(long long)\n";
  } else if LIBIPC_UNLIKELY(xx < sizeof(char)) {
    std::cout << "sizeof(int) < sizeof(char)\n";
  } else {
    std::cout << "sizeof(int) < whatever\n";
  }
  SUCCEED();
}

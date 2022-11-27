#define _CRT_SECURE_NO_WARNINGS
#include <iostream>

#include "gtest/gtest.h"

#include "libimp/fmt.h"

TEST(fmt, operator) {
  auto a = imp::spec("hello")(123);
  EXPECT_STREQ(a.fstr.data(), "hello");
  EXPECT_EQ(a.param , 123);

  auto b = imp::spec("hello")("world");
  EXPECT_STREQ(b.fstr.data(), "hello");
  EXPECT_STREQ(b.param , "world");
}

TEST(fmt, to_string) {
  /// @brief string
  EXPECT_EQ(imp::to_string(""), "");
  EXPECT_EQ(imp::to_string("%what%"), "%what%");
  EXPECT_EQ(imp::to_string("%what%", "10") , "    %what%");
  EXPECT_EQ(imp::to_string("%what%", "-10"), "%what%    ");

  /// @brief character
  EXPECT_EQ(imp::to_string('A'), "A");
  EXPECT_EQ(imp::to_string(L'A'), "A");
  EXPECT_EQ(imp::to_string(u'A'), "A");
  EXPECT_EQ(imp::to_string(U'A'), "A");

  /// @brief numeric
  EXPECT_EQ(imp::to_string((signed char)123), "123");
  EXPECT_EQ(imp::to_string((signed char)-321), "-65");
  EXPECT_EQ(imp::to_string((unsigned char)123), "123");
  EXPECT_EQ(imp::to_string((unsigned char)321), "65");
  EXPECT_EQ(imp::to_string((short)123), "123");
  EXPECT_EQ(imp::to_string((short)-321), "-321");
  EXPECT_EQ(imp::to_string((unsigned short)123), "123");
  EXPECT_EQ(imp::to_string((unsigned short)321), "321");
  EXPECT_EQ(imp::to_string((short)123123), "-7949");
  EXPECT_EQ(imp::to_string((short)-321321), "6359");
  EXPECT_EQ(imp::to_string((unsigned short)123123), "57587");
  EXPECT_EQ(imp::to_string((unsigned short)321321), "59177");
  EXPECT_EQ(imp::to_string(123123), "123123");
  EXPECT_EQ(imp::to_string(-321321), "-321321");
  EXPECT_EQ(imp::to_string(123123u), "123123");
  EXPECT_EQ(imp::to_string(321321u), "321321");
  EXPECT_EQ(imp::to_string(123123ll), "123123");
  EXPECT_EQ(imp::to_string(-321321ll), "-321321");
  EXPECT_EQ(imp::to_string(123123ull), "123123");
  EXPECT_EQ(imp::to_string(321321ull), "321321");
  EXPECT_EQ(imp::to_string(123123, "x"), "1e0f3");
  EXPECT_EQ(imp::to_string(123123u, "x"), "1e0f3");
  EXPECT_EQ(imp::to_string(123123123123ll, "X"), "1CAAB5C3B3");
  EXPECT_EQ(imp::to_string(123123123123ull, "X"), "1CAAB5C3B3");

  /// @brief floating point
  EXPECT_EQ(imp::to_string(123.123f, ".3"), "123.123");
  EXPECT_EQ(imp::to_string(123.123, "010.5"), "0123.12300");
  EXPECT_EQ(imp::to_string(123.123l, "010.6"), "123.123000");
  EXPECT_EQ(imp::to_string(1.5, "e"), "1.500000e+00");
  EXPECT_EQ(imp::to_string(1.5, "E"), "1.500000E+00");
  double r = 0.0;
  std::cout << imp::to_string(0.0/r) << "\n";
  std::cout << imp::to_string(1.0/r) << "\n";
  SUCCEED();

  /// @brief pointer
  EXPECT_EQ(imp::to_string(nullptr), "null");
  int *p = (int *)0x0f013a04;
  std::cout << imp::to_string((void *)p) << "\n";
  SUCCEED();

  /// @brief date and time
  auto tp = std::chrono::system_clock::now();
  auto tt = std::chrono::system_clock::to_time_t(tp);
  auto tm = *std::localtime(&tt);
  std::cout << imp::to_string(tm) << "\n";
  EXPECT_EQ(imp::to_string(tm), imp::to_string(tp));
  SUCCEED();
}

TEST(fmt, fmt) {
  auto s = imp::fmt("hello", " ", "world", ".");
  EXPECT_EQ(s, "hello world.");
  std::cout << imp::fmt('[', std::chrono::system_clock::now(), "] ", s) << "\n";
}
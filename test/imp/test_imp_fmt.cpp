#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <tuple>

#include "gtest/gtest.h"

#include "libimp/fmt.h"

TEST(fmt, spec) {
  EXPECT_STREQ(imp::spec("hello")(123).fstr.data(), "hello");
  EXPECT_EQ(imp::spec("hello")(123).param , 123);
  EXPECT_STREQ(imp::spec("hello")("world").fstr.data(), "hello");
  EXPECT_STREQ(imp::spec("hello")("world").param , "world");
}

TEST(fmt, to_string) {
  std::string joined;
  imp::fmt_context ctx(joined);

  /// @brief string
  EXPECT_TRUE(imp::to_string(ctx, "")); ctx.finish();
  EXPECT_EQ(joined, "");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, "%what%")); ctx.finish();
  EXPECT_EQ(joined, "%what%");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, "%what%", "10")); ctx.finish();
  EXPECT_EQ(joined, "    %what%");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, "%what%", "-10")); ctx.finish();
  EXPECT_EQ(joined, "%what%    ");

  /// @brief character
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 'A')); ctx.finish();
  EXPECT_EQ(joined, "A");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, L'A')); ctx.finish();
  EXPECT_EQ(joined, "A");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, u'A')); ctx.finish();
  EXPECT_EQ(joined, "A");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, U'A')); ctx.finish();
  EXPECT_EQ(joined, "A");

  /// @brief numeric
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (signed char)123)); ctx.finish(); EXPECT_EQ(joined, "123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (signed char)-321)); ctx.finish(); EXPECT_EQ(joined, "-65");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (unsigned char)123)); ctx.finish(); EXPECT_EQ(joined, "123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (unsigned char)321)); ctx.finish(); EXPECT_EQ(joined, "65");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (short)123)); ctx.finish(); EXPECT_EQ(joined, "123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (short)-321)); ctx.finish(); EXPECT_EQ(joined, "-321");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (unsigned short)123)); ctx.finish(); EXPECT_EQ(joined, "123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (unsigned short)321)); ctx.finish(); EXPECT_EQ(joined, "321");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (short)123123)); ctx.finish(); EXPECT_EQ(joined, "-7949");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (short)-321321)); ctx.finish(); EXPECT_EQ(joined, "6359");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (unsigned short)123123)); ctx.finish(); EXPECT_EQ(joined, "57587");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (unsigned short)321321)); ctx.finish(); EXPECT_EQ(joined, "59177");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123)); ctx.finish(); EXPECT_EQ(joined, "123123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, -321321)); ctx.finish(); EXPECT_EQ(joined, "-321321");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123u)); ctx.finish(); EXPECT_EQ(joined, "123123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 321321u)); ctx.finish(); EXPECT_EQ(joined, "321321");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123ll)); ctx.finish(); EXPECT_EQ(joined, "123123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, -321321ll)); ctx.finish(); EXPECT_EQ(joined, "-321321");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123ull)); ctx.finish(); EXPECT_EQ(joined, "123123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 321321ull)); ctx.finish(); EXPECT_EQ(joined, "321321");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123, "x")); ctx.finish(); EXPECT_EQ(joined, "1e0f3");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123u, "x")); ctx.finish(); EXPECT_EQ(joined, "1e0f3");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123123123ll, "X")); ctx.finish(); EXPECT_EQ(joined, "1CAAB5C3B3");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123123123123ull, "X")); ctx.finish(); EXPECT_EQ(joined, "1CAAB5C3B3");

  /// @brief floating point
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123.123f, ".3")); ctx.finish(); EXPECT_EQ(joined, "123.123");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123.123, "010.5")); ctx.finish(); EXPECT_EQ(joined, "0123.12300");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 123.123l, "010.6")); ctx.finish(); EXPECT_EQ(joined, "123.123000");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 1.5, "e")); ctx.finish(); EXPECT_EQ(joined, "1.500000e+00");
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 1.5, "E")); ctx.finish(); EXPECT_EQ(joined, "1.500000E+00");
  double r = 0.0;
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 0.0/r)); ctx.finish();
  std::cout << joined << "\n";
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, 1.0/r)); ctx.finish();
  std::cout << joined << "\n";

  /// @brief pointer
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, nullptr)); ctx.finish(); EXPECT_EQ(joined, "null");
  int *p = (int *)0x0f013a04;
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, (void *)p)); ctx.finish();
  std::cout << joined << "\n";

  /// @brief date and time
  auto tp = std::chrono::system_clock::now();
  auto tt = std::chrono::system_clock::to_time_t(tp);
  auto tm = *std::localtime(&tt);
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, tm)); ctx.finish();
  std::cout << joined << "\n";
  std::string tm_str = joined;
  ctx.reset(); EXPECT_TRUE(imp::to_string(ctx, tp)); ctx.finish();
  EXPECT_EQ(tm_str, joined);
}

TEST(fmt, fmt) {
  auto s = imp::fmt("hello", " ", "world", ".");
  EXPECT_EQ(s, "hello world.");
  std::cout << imp::fmt('[', std::chrono::system_clock::now(), "] ", s) << "\n";
}

namespace {

class foo {};

bool tag_invoke(decltype(imp::fmt_to), imp::fmt_context &, foo arg) noexcept(false) {
  throw arg;
  return {};
}

} // namespace

TEST(fmt, throw) {
  EXPECT_THROW(std::ignore = imp::fmt(foo{}), foo);
}
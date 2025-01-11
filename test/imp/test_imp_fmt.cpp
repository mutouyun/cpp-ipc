#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <tuple>
#include <cstring>

#include "test.h"

#include "libipc/imp/fmt.h"
#include "libipc/imp/byte.h"
#include "libipc/imp/span.h"
#include "libipc/imp/result.h"

TEST(fmt, spec) {
  EXPECT_STREQ(ipc::spec("hello")(123).fstr.data(), "hello");
  EXPECT_EQ(ipc::spec("hello")(123).param , 123);
  EXPECT_STREQ(ipc::spec("hello")("world").fstr.data(), "hello");
  EXPECT_STREQ(ipc::spec("hello")("world").param , "world");
}

TEST(fmt, to_string) {
  std::string joined;
  ipc::fmt_context ctx(joined);

  auto check = [&](auto &&txt, auto &&...val) {
    ctx.reset();
    EXPECT_TRUE(ipc::to_string(ctx, std::forward<decltype(val)>(val)...));
    ctx.finish();
    EXPECT_EQ(joined, std::forward<decltype(txt)>(txt));
  };

  /// \brief string
  check("", "");
  check("%what%", "%what%");
  check("    %what%", "%what%", "10");
  check("%what%    ", "%what%", "-10");

  /// \brief character
  check("A", 'A');
  check("A", L'A');
  check("A", u'A');
  check("A", U'A');

  /// \brief numeric
  check("123"       , (signed char)123      );
  check("-65"       , (signed char)-321     );
  check("123"       , (unsigned char)123    );
  check("65"        , (unsigned char)321    );
  check("123"       , (short)123            );
  check("-321"      , (short)-321           );
  check("123"       , (unsigned short)123   );
  check("321"       , (unsigned short)321   );
  check("-7949"     , (short)123123         );
  check("6359"      , (short)-321321        );
  check("57587"     , (unsigned short)123123);
  check("59177"     , (unsigned short)321321);
  check("123123"    , 123123                );
  check("-321321"   , -321321               );
  check("123123"    , 123123u               );
  check("321321"    , 321321u               );
  check("123123"    , 123123ll              );
  check("-321321"   , -321321ll             );
  check("123123"    , 123123ull             );
  check("321321"    , 321321ull             );
  check("1e0f3"     , 123123, "x"           );
  check("1e0f3"     , 123123u, "x"          );
  check("1CAAB5C3B3", 123123123123ll, "X"   );
  check("1CAAB5C3B3", 123123123123ull, "X"  );

  /// \brief floating point
  check("123.123"     , 123.123f, ".3");
  check("0123.12300"  , 123.123, "010.5");
  check("123.123000"  , 123.123l, "010.6");
  check("1.500000e+00", 1.5, "e");
  check("1.500000E+00", 1.5, "E");
  double r = 0.0;
  ctx.reset(); EXPECT_TRUE(ipc::to_string(ctx, 0.0/r)); ctx.finish();
  std::cout << joined << "\n";
  ctx.reset(); EXPECT_TRUE(ipc::to_string(ctx, 1.0/r)); ctx.finish();
  std::cout << joined << "\n";

  /// \brief pointer
  check("null", nullptr);
  int *p = (int *)0x0f013a04;
  ctx.reset(); EXPECT_TRUE(ipc::to_string(ctx, (void *)p)); ctx.finish();
  std::cout << joined << "\n";

  /// \brief date and time
  auto tp = std::chrono::system_clock::now();
  auto tt = std::chrono::system_clock::to_time_t(tp);
  auto tm = *std::localtime(&tt);
  ctx.reset(); EXPECT_TRUE(ipc::to_string(ctx, tm)); ctx.finish();
  std::cout << joined << "\n";
  std::string tm_str = joined;
  ctx.reset(); EXPECT_TRUE(ipc::to_string(ctx, tp)); ctx.finish();
  EXPECT_EQ(tm_str, joined);
}

TEST(fmt, fmt) {
  char const txt[] = "hello world.";

  /// \brief hello world
  auto s = ipc::fmt("hello", " ", "world", ".");
  EXPECT_EQ(s, txt);

  /// \brief chrono
  std::cout << ipc::fmt('[', std::chrono::system_clock::now(), "] ", s) << "\n";

  /// \brief long string
  s = ipc::fmt(ipc::spec("4096")(txt));
  std::string test(4096, ' ');
  std::memcpy(&test[test.size() - sizeof(txt) + 1], txt, sizeof(txt) - 1);
  EXPECT_EQ(s, test);

  EXPECT_EQ(ipc::fmt("", 1, "", '2', "", 3.0), "123.000000");
  std::string empty;
  EXPECT_EQ(ipc::fmt(empty, 1, "", '2', "", 3.0), "123.000000");
  EXPECT_EQ(ipc::fmt(empty, 1, empty, '2', "", 3.0), "123.000000");
  EXPECT_EQ(ipc::fmt("", 1, empty, '2', empty, 3.0), "123.000000");
  EXPECT_EQ(ipc::fmt("", 1, "", '2', empty), "12");
}

namespace {

class foo {};

bool tag_invoke(decltype(ipc::fmt_to), ipc::fmt_context &, foo arg) noexcept(false) {
  throw arg;
  return {};
}

} // namespace

TEST(fmt, throw) {
  EXPECT_THROW(std::ignore = ipc::fmt(foo{}), foo);
}

TEST(fmt, byte) {
  {
    ipc::byte b1{}, b2(31);
    EXPECT_EQ(ipc::fmt(b1), "00");
    EXPECT_EQ(ipc::fmt(b2), "1f");
    EXPECT_EQ(ipc::fmt(ipc::spec("03X")(b2)), "01F");
  }
  {
    ipc::byte bs[] {31, 32, 33, 34, 35, 36, 37, 38};
    EXPECT_EQ(ipc::fmt(ipc::make_span(bs)), "1f 20 21 22 23 24 25 26");
  }
}

TEST(fmt, span) {
  EXPECT_EQ(ipc::fmt(ipc::span<int>{}), "");
  EXPECT_EQ(ipc::fmt(ipc::make_span({1, 3, 2, 4, 5, 6, 7})), "1 3 2 4 5 6 7");
}

TEST(fmt, result) {
  {
    ipc::result<std::uint64_t> r1;
    EXPECT_EQ(ipc::fmt(r1), ipc::fmt("fail, error = ", std::error_code(-1, std::generic_category())));
    ipc::result<std::uint64_t> r2(65537);
    EXPECT_EQ(ipc::fmt(r2), "succ, value = 65537");
    ipc::result<std::uint64_t> r3(0);
    EXPECT_EQ(ipc::fmt(r3), "succ, value = 0");
  }
  {
    ipc::result<int> r0;
    EXPECT_EQ(ipc::fmt(r0), ipc::fmt(ipc::result<std::uint64_t>()));
    ipc::result<int> r1 {std::error_code(-1, std::generic_category())};
    EXPECT_EQ(ipc::fmt(r1), ipc::fmt("fail, error = ", std::error_code(-1, std::generic_category())));

    ipc::result<void *> r2 {&r1};
    EXPECT_EQ(ipc::fmt(r2), ipc::fmt("succ, value = ", (void *)&r1));

    int aaa {};
    ipc::result<int *> r3 {&aaa};
    EXPECT_EQ(ipc::fmt(r3), ipc::fmt("succ, value = ", (void *)&aaa));
    ipc::result<int *> r4 {nullptr};
    EXPECT_EQ(ipc::fmt(r4), ipc::fmt("fail, error = ", std::error_code(-1, std::generic_category())));
    r4 = std::error_code(1234, std::generic_category());
    EXPECT_EQ(ipc::fmt(r4), ipc::fmt("fail, error = ", std::error_code(1234, std::generic_category())));
    ipc::result<int *> r5;
    EXPECT_EQ(ipc::fmt(r5), ipc::fmt("fail, error = ", std::error_code(-1, std::generic_category())));
  }
  {
    ipc::result<std::int64_t> r1 {-123};
    EXPECT_EQ(ipc::fmt(r1), ipc::fmt("succ, value = ", -123));
  }
  {
    ipc::result<void> r1;
    EXPECT_EQ(ipc::fmt(r1), ipc::fmt("fail, error = ", std::error_code(-1, std::generic_category())));
    r1 = std::error_code{};
    EXPECT_TRUE(r1);
    EXPECT_EQ(ipc::fmt(r1), ipc::fmt("succ, error = ", std::error_code()));
  }
}

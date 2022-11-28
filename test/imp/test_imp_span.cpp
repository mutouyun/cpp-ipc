
#include <string>
#include <memory>

#include "gtest/gtest.h"

#include "libimp/span.h"
#include "libimp/countof.h"
#include "libimp/byte.h"
#include "libimp/fmt.h"

TEST(span, to_address) {
  int *a = new int;
  EXPECT_EQ(imp::detail::to_address(a), a);
  std::unique_ptr<int> b {a};
  EXPECT_EQ(imp::detail::to_address(b), a);
}

TEST(span, span) {
  auto test_proc = [](auto &&buf, auto &&sp) {
    EXPECT_EQ(imp::countof(buf), sp.size());
    EXPECT_EQ(sizeof(buf[0]) * imp::countof(buf), sp.size_bytes());
    for (int i = 0; i < sp.size(); ++i) {
      EXPECT_EQ(buf[i], sp[i]);
    }
  };
  {
    int buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto sp = imp::make_span(buf);
    test_proc(buf, sp);
    test_proc(imp::make_span({0, 1, 2})   , sp.first(3));
    test_proc(imp::make_span({6, 7, 8, 9}), sp.last(4));
    test_proc(imp::make_span({3, 4, 5, 6}), sp.subspan(3, 4));
    test_proc(imp::make_span({3, 4, 5, 6, 7, 8, 9}), sp.subspan(3));
  }
  {
    std::vector<int> buf = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto sp = imp::make_span(buf);
    imp::span<int> sp2 {buf.begin(), buf.end()};
    EXPECT_EQ(sp, sp2);
    test_proc(buf, sp);
    test_proc(imp::make_span({0, 1, 2})   , sp.first(3));
    test_proc(imp::make_span({6, 7, 8, 9}), sp.last(4));
    test_proc(imp::make_span({3, 4, 5, 6}), sp.subspan(3, 4));
    test_proc(imp::make_span({3, 4, 5, 6, 7, 8, 9}), sp.subspan(3));
    test_proc(imp::make_span((imp::byte *)sp.data(), sp.size_bytes()), imp::as_bytes(sp));
  }
  {
    std::string buf = "0123456789";
    auto sp = imp::make_span(buf);
    // if (sp == imp::make_span({nullptr})) {}
    test_proc(buf, sp);
    test_proc(imp::make_span("012", 3) , sp.first(3));
    test_proc(imp::make_span("6789", 4), sp.last(4));
    test_proc(imp::make_span("3456", 4), sp.subspan(3, 4));
    test_proc(imp::make_span("3456789", 7), sp.subspan(3));
  }
}

TEST(span, fmt) {
  EXPECT_EQ(imp::fmt(imp::span<int>{}), "");
  EXPECT_EQ(imp::fmt(imp::make_span({1, 3, 2, 4, 5, 6, 7})), "1 3 2 4 5 6 7");
}
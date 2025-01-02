
#include "test.h"

#include "libipc/imp/span.h"
#include "libipc/imp/generic.h"
#include "libipc/imp/byte.h"

TEST(span, to_address) {
  int *a = new int;
  EXPECT_EQ(ipc::detail_span::to_address(a), a);
  std::unique_ptr<int> b {a};
  EXPECT_EQ(ipc::detail_span::to_address(b), a);
}

TEST(span, span) {
  auto test_proc = [](auto &&buf, auto &&sp) {
    EXPECT_EQ(ipc::countof(buf), sp.size());
    EXPECT_EQ(sizeof(buf[0]) * ipc::countof(buf), sp.size_bytes());
    for (std::size_t i = 0; i < sp.size(); ++i) {
      EXPECT_EQ(buf[i], sp[i]);
    }
  };
  {
    int buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto sp = ipc::make_span(buf);
    test_proc(buf, sp);
    test_proc(ipc::make_span({0, 1, 2})   , sp.first(3));
    test_proc(ipc::make_span({6, 7, 8, 9}), sp.last(4));
    test_proc(ipc::make_span({3, 4, 5, 6}), sp.subspan(3, 4));
    test_proc(ipc::make_span({3, 4, 5, 6, 7, 8, 9}), sp.subspan(3));
  }
  {
    std::vector<int> buf = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto sp = ipc::make_span(buf);
    ipc::span<int> sp2 {buf.begin(), buf.end()};
    EXPECT_EQ(sp, sp2);
    test_proc(buf, sp);
    test_proc(ipc::make_span({0, 1, 2})   , sp.first(3));
    test_proc(ipc::make_span({6, 7, 8, 9}), sp.last(4));
    test_proc(ipc::make_span({3, 4, 5, 6}), sp.subspan(3, 4));
    test_proc(ipc::make_span({3, 4, 5, 6, 7, 8, 9}), sp.subspan(3));
    test_proc(ipc::make_span((ipc::byte *)sp.data(), sp.size_bytes()), ipc::as_bytes(sp));
  }
  {
    std::string buf = "0123456789";
    auto sp = ipc::make_span(buf);
    // if (sp == ipc::make_span({nullptr})) {}
    test_proc(buf, sp);
    test_proc(ipc::make_span("012", 3) , sp.first(3));
    test_proc(ipc::make_span("6789", 4), sp.last(4));
    test_proc(ipc::make_span("3456", 4), sp.subspan(3, 4));
    test_proc(ipc::make_span("3456789", 7), sp.subspan(3));
  }
}

TEST(span, construct) {
  struct test_imp_span_construct {
    /* data */
    int  size() const noexcept { return sizeof(this); }
    auto Data() const noexcept { return this; }
  } d1;
  ipc::span<test_imp_span_construct const> sp {d1};
  // ipc::span<int const> spp {d1};
  EXPECT_EQ(sp.size(), d1.size());
  EXPECT_EQ(sp.data(), d1.Data());
}

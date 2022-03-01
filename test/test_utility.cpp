
#include <type_traits>  // std::aligned_storage_t
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

#include "libipc/utility/construct.h"
#include "libipc/utility/pimpl.h"
#include "libipc/utility/countof.h"

TEST(utility, construct) {
  struct Foo {
    int a_;
    short b_;
    char c_;
  };
  std::aligned_storage_t<sizeof(Foo)> foo;
  Foo *pfoo = ipc::construct<Foo>(&foo, 123, short{321}, '1');
  EXPECT_EQ(pfoo->a_, 123);
  EXPECT_EQ(pfoo->b_, 321);
  EXPECT_EQ(pfoo->c_, '1');
  ipc::destroy(pfoo);

  static int bar_test_flag = 0;
  struct Bar : Foo {
    Bar(int a, short b, char c)
        : Foo{a, b, c} {
      ++bar_test_flag;
    }
    ~Bar() { --bar_test_flag; }
  };
  std::aligned_storage_t<sizeof(Bar)> bar;
  Bar *pbar = ipc::construct<Bar>(&bar, 123, short(321), '1');
  EXPECT_EQ(pbar->a_, 123);
  EXPECT_EQ(pbar->b_, 321);
  EXPECT_EQ(pbar->c_, '1');
  EXPECT_EQ(bar_test_flag, 1);
  ipc::destroy(pbar);
  EXPECT_EQ(bar_test_flag, 0);

  std::aligned_storage_t<sizeof(Bar)> bars[3];
  for (auto &b : bars) {
    auto pb = ipc::construct<Bar>(&b, 321, short(123), '3');
    EXPECT_EQ(pb->a_, 321);
    EXPECT_EQ(pb->b_, 123);
    EXPECT_EQ(pb->c_, '3');
  }
  //EXPECT_EQ(bar_test_flag, ipc::countof(bars));
  ipc::destroy(reinterpret_cast<Bar(*)[3]>(&bars));
  EXPECT_EQ(bar_test_flag, 0);
}

namespace {

struct Foo : ipc::pimpl::Obj<Foo> {
  int *pi_;
  Foo(int *pi) : pi_(pi) {}
};

struct Bar : ipc::pimpl::Obj<Bar> {
  int *pi_;
  int *pj_;
  Bar(int *pi, int *pj) : pi_(pi), pj_(pj) {}
};

} // namespace

TEST(utility, pimpl_is_comfortable) {
  EXPECT_TRUE ((ipc::pimpl::is_comfortable<std::int32_t, std::int64_t>::value));
  EXPECT_TRUE ((ipc::pimpl::is_comfortable<std::int64_t, std::int64_t>::value));
  EXPECT_FALSE((ipc::pimpl::is_comfortable<std::int64_t, std::int32_t>::value));

  EXPECT_TRUE ((ipc::pimpl::is_comfortable<Foo>::value));
  EXPECT_FALSE((ipc::pimpl::is_comfortable<Bar>::value));
}

TEST(utility, pimpl_inherit) {
  int i = 123;
  Foo *pfoo = Foo::make(&i);
  EXPECT_EQ(ipc::pimpl::get(pfoo)->pi_, &i);
  pfoo->clear();

  int j = 321;
  Bar *pbar = Bar::make(&i, &j);
  EXPECT_EQ(ipc::pimpl::get(pbar)->pi_, &i);
  EXPECT_EQ(ipc::pimpl::get(pbar)->pj_, &j);
  pbar->clear();
}

TEST(utility, countof) {
  struct {
    constexpr int Size() const noexcept { return 3; }
  } sv;
  EXPECT_FALSE(ipc::detail::countof_trait_has_size<decltype(sv)>::value);
  EXPECT_TRUE (ipc::detail::countof_trait_has_Size<decltype(sv)>::value);

  std::vector<int> vec {1, 2, 3, 4, 5};
  int arr[] {7, 6, 5, 4, 3, 2, 1};
  EXPECT_EQ(ipc::countof(sv) , sv.Size());
  EXPECT_EQ(ipc::countof(vec), vec.size());
  EXPECT_EQ(ipc::countof(arr), sizeof(arr) / sizeof(arr[0]));
}

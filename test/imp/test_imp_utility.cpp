
#include <type_traits>  // std::aligned_storage_t
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

#include "libimp/uninitialized.h"
#include "libimp/pimpl.h"
#include "libimp/countof.h"
#include "libimp/dataof.h"
#include "libimp/horrible_cast.h"
#include "libimp/detect_plat.h"
#include "libimp/generic.h"

TEST(utility, construct) {
  struct Foo {
    int a_;
    short b_;
    char c_;
  };
  std::aligned_storage_t<sizeof(Foo)> foo;
  Foo *pfoo = imp::construct<Foo>(&foo, 123, short{321}, '1');
  EXPECT_EQ(pfoo->a_, 123);
  EXPECT_EQ(pfoo->b_, 321);
  EXPECT_EQ(pfoo->c_, '1');
  imp::destroy(pfoo);

  static int bar_test_flag = 0;
  struct Bar : Foo {
    Bar(int a, short b, char c)
        : Foo{a, b, c} {
      ++bar_test_flag;
    }
    ~Bar() { --bar_test_flag; }
  };
  std::aligned_storage_t<sizeof(Bar)> bar;
  Bar *pbar = imp::construct<Bar>(&bar, 123, short(321), '1');
  EXPECT_EQ(pbar->a_, 123);
  EXPECT_EQ(pbar->b_, 321);
  EXPECT_EQ(pbar->c_, '1');
  EXPECT_EQ(bar_test_flag, 1);
  imp::destroy(pbar);
  EXPECT_EQ(bar_test_flag, 0);

  std::aligned_storage_t<sizeof(Bar)> bars[3];
  for (auto &b : bars) {
    auto pb = imp::construct<Bar>(&b, 321, short(123), '3');
    EXPECT_EQ(pb->a_, 321);
    EXPECT_EQ(pb->b_, 123);
    EXPECT_EQ(pb->c_, '3');
  }
  //EXPECT_EQ(bar_test_flag, imp::countof(bars));
  imp::destroy(reinterpret_cast<Bar(*)[3]>(&bars));
  EXPECT_EQ(bar_test_flag, 0);
}

namespace {

struct Foo : imp::pimpl::Obj<Foo> {
  int *pi_;
  Foo(int *pi) : pi_(pi) {}
};

struct Bar : imp::pimpl::Obj<Bar> {
  int *pi_;
  int *pj_;
  Bar(int *pi, int *pj) : pi_(pi), pj_(pj) {}
};

} // namespace

TEST(utility, pimpl_is_comfortable) {
  EXPECT_TRUE ((imp::pimpl::is_comfortable<std::int32_t, std::int64_t>::value));
  EXPECT_TRUE ((imp::pimpl::is_comfortable<std::int64_t, std::int64_t>::value));
  EXPECT_FALSE((imp::pimpl::is_comfortable<std::int64_t, std::int32_t>::value));

  EXPECT_TRUE ((imp::pimpl::is_comfortable<Foo>::value));
  EXPECT_FALSE((imp::pimpl::is_comfortable<Bar>::value));
}

TEST(utility, pimpl_inherit) {
  int i = 123;
  Foo *pfoo = Foo::make(&i);
  EXPECT_EQ(imp::pimpl::get(pfoo)->pi_, &i);
  pfoo->clear();

  int j = 321;
  Bar *pbar = Bar::make(&i, &j);
  EXPECT_EQ(imp::pimpl::get(pbar)->pi_, &i);
  EXPECT_EQ(imp::pimpl::get(pbar)->pj_, &j);
  pbar->clear();
}

TEST(utility, countof) {
  struct {
    constexpr int Size() const noexcept { return 3; }
  } sv;
  EXPECT_FALSE(imp::detail_countof::trait_has_size<decltype(sv)>::value);
  EXPECT_TRUE (imp::detail_countof::trait_has_Size<decltype(sv)>::value);

  std::vector<int> vec {1, 2, 3, 4, 5};
  int arr[] {7, 6, 5, 4, 3, 2, 1};
  auto il = {9, 7, 6, 4, 3, 1, 5};
  EXPECT_EQ(imp::countof(sv) , sv.Size());
  EXPECT_EQ(imp::countof(vec), vec.size());
  EXPECT_EQ(imp::countof(arr), sizeof(arr) / sizeof(arr[0]));
  EXPECT_EQ(imp::countof(il) , il.size());
}

TEST(utility, dataof) {
  struct {
    int *Data() const noexcept { return (int *)this; }
  } sv;
  EXPECT_FALSE(imp::detail_dataof::trait_has_data<decltype(sv)>::value);
  EXPECT_TRUE (imp::detail_dataof::trait_has_Data<decltype(sv)>::value);

  std::vector<int> vec {1, 2, 3, 4, 5};
  int arr[] {7, 6, 5, 4, 3, 2, 1};
  auto il = {9, 7, 6, 4, 3, 1, 5};
  EXPECT_EQ(imp::dataof(sv) , sv.Data());
  EXPECT_EQ(imp::dataof(vec), vec.data());
  EXPECT_EQ(imp::dataof(arr), arr);
  EXPECT_EQ(imp::dataof(il) , il.begin());
}

TEST(utility, horrible_cast) {
  struct A {
    int a_;
  } a {123};

  struct B {
    char a_[sizeof(int)];
  } b = imp::horrible_cast<B>(a);

  EXPECT_EQ(b.a_[1], 0);
  EXPECT_EQ(b.a_[2], 0);
#if LIBIMP_ENDIAN_LIT
  EXPECT_EQ(b.a_[0], 123);
  EXPECT_EQ(b.a_[3], 0);
#else
  EXPECT_EQ(b.a_[3], 123);
  EXPECT_EQ(b.a_[0], 0);
#endif

#if LIBIMP_ENDIAN_LIT
  EXPECT_EQ(imp::horrible_cast<std::uint32_t>(0xff00'0000'0001ll), 1);
#else
  EXPECT_EQ(imp::horrible_cast<std::uint32_t>(0xff00'0000'0001ll), 0xff00);
#endif
}

#if defined(LIBIMP_CPP_17)
TEST(utility, in_place) {
  EXPECT_TRUE((std::is_same<std::in_place_t, imp::in_place_t>::value));
  [](imp::in_place_t) {}(std::in_place);
  [](std::in_place_t) {}(imp::in_place);
}
#endif/*LIBIMP_CPP_17*/

TEST(utility, copy_cvref) {
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int   , long>, long   >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int & , long>, long & >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int &&, long>, long &&>()));

  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int const   , long>, long const   >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int const & , long>, long const & >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int const &&, long>, long const &&>()));

  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int volatile   , long>, long volatile   >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int volatile & , long>, long volatile & >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int volatile &&, long>, long volatile &&>()));

  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int const volatile   , long>, long const volatile   >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int const volatile & , long>, long const volatile & >()));
  EXPECT_TRUE((std::is_same<imp::copy_cvref_t<int const volatile &&, long>, long const volatile &&>()));
}

#include "test.h"

#include "libipc/imp/uninitialized.h"

TEST(uninitialized, construct) {
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

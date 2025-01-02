
#include "test.h"

#include "libipc/imp/generic.h"
#include "libipc/imp/detect_plat.h"

TEST(generic, countof) {
  struct {
    constexpr int Size() const noexcept { return 3; }
  } sv;
  EXPECT_FALSE(ipc::detail_countof::trait_has_size<decltype(sv)>::value);
  EXPECT_TRUE (ipc::detail_countof::trait_has_Size<decltype(sv)>::value);

  std::vector<int> vec {1, 2, 3, 4, 5};
  int arr[] {7, 6, 5, 4, 3, 2, 1};
  auto il = {9, 7, 6, 4, 3, 1, 5};
  EXPECT_EQ(ipc::countof(sv) , sv.Size());
  EXPECT_EQ(ipc::countof(vec), vec.size());
  EXPECT_EQ(ipc::countof(arr), sizeof(arr) / sizeof(arr[0]));
  EXPECT_EQ(ipc::countof(il) , il.size());
}

TEST(generic, dataof) {
  struct {
    int *Data() const noexcept { return (int *)this; }
  } sv;
  EXPECT_FALSE(ipc::detail_dataof::trait_has_data<decltype(sv)>::value);
  EXPECT_TRUE (ipc::detail_dataof::trait_has_Data<decltype(sv)>::value);

  std::vector<int> vec {1, 2, 3, 4, 5};
  int arr[] {7, 6, 5, 4, 3, 2, 1};
  auto il = {9, 7, 6, 4, 3, 1, 5};
  EXPECT_EQ(ipc::dataof(sv) , sv.Data());
  EXPECT_EQ(ipc::dataof(vec), vec.data());
  EXPECT_EQ(ipc::dataof(arr), arr);
  EXPECT_EQ(ipc::dataof(il) , il.begin());
}

TEST(generic, horrible_cast) {
  struct A {
    int a_;
  } a {123};

  struct B {
    char a_[sizeof(int)];
  } b = ipc::horrible_cast<B>(a);

  EXPECT_EQ(b.a_[1], 0);
  EXPECT_EQ(b.a_[2], 0);
#if LIBIPC_ENDIAN_LIT
  EXPECT_EQ(b.a_[0], 123);
  EXPECT_EQ(b.a_[3], 0);
#else
  EXPECT_EQ(b.a_[3], 123);
  EXPECT_EQ(b.a_[0], 0);
#endif

#if LIBIPC_ENDIAN_LIT
  EXPECT_EQ(ipc::horrible_cast<std::uint32_t>(0xff00'0000'0001ll), 1);
#else
  EXPECT_EQ(ipc::horrible_cast<std::uint32_t>(0xff00'0000'0001ll), 0xff00);
#endif
}

#if defined(LIBIPC_CPP_17)
TEST(generic, in_place) {
  EXPECT_TRUE((std::is_same<std::in_place_t, ipc::in_place_t>::value));
  [](ipc::in_place_t) {}(std::in_place);
  [](std::in_place_t) {}(ipc::in_place);
}
#endif/*LIBIPC_CPP_17*/

TEST(generic, copy_cvref) {
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int   , long>, long   >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int & , long>, long & >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int &&, long>, long &&>()));

  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int const   , long>, long const   >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int const & , long>, long const & >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int const &&, long>, long const &&>()));

  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int volatile   , long>, long volatile   >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int volatile & , long>, long volatile & >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int volatile &&, long>, long volatile &&>()));

  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int const volatile   , long>, long const volatile   >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int const volatile & , long>, long const volatile & >()));
  EXPECT_TRUE((std::is_same<ipc::copy_cvref_t<int const volatile &&, long>, long const volatile &&>()));
}

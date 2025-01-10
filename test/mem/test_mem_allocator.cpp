
#include <vector>
#include <utility>

#include "test.h"

#if defined(LIBIPC_CPP_17) && defined(__cpp_lib_memory_resource)
# include <memory_resource>
#endif

#include "libipc/mem/allocator.h"
#include "libipc/mem/memory_resource.h"

TEST(allocator, construct) {
  ipc::mem::allocator alc;
  SUCCEED();
}

TEST(allocator, construct_value_initialization) {
  ipc::mem::allocator alc{};
  auto p = alc.allocate(128);
  EXPECT_NE(p, nullptr);
  EXPECT_NO_THROW(alc.deallocate(p, 128));
}

namespace {

class dummy_resource {
public:
  void *allocate(std::size_t, std::size_t = 0) noexcept {
    return nullptr;
  }
  void deallocate(void *, std::size_t, std::size_t = 0) noexcept {
    return;
  }
};

} // namespace

TEST(allocator, memory_resource_traits) {
  EXPECT_FALSE(ipc::mem::has_allocate<void>::value);
  EXPECT_FALSE(ipc::mem::has_allocate<int>::value);
  EXPECT_FALSE(ipc::mem::has_allocate<std::vector<int>>::value);
  EXPECT_FALSE(ipc::mem::has_allocate<std::allocator<int>>::value);
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_memory_resource)
  EXPECT_TRUE (ipc::mem::has_allocate<std::ipc::mem::memory_resource>::value);
  EXPECT_TRUE (ipc::mem::has_allocate<std::ipc::mem::polymorphic_allocator<int>>::value);
#endif

  EXPECT_FALSE(ipc::mem::has_deallocate<void>::value);
  EXPECT_FALSE(ipc::mem::has_deallocate<int>::value);
  EXPECT_FALSE(ipc::mem::has_deallocate<std::vector<int>>::value);
  EXPECT_FALSE(ipc::mem::has_deallocate<std::allocator<int>>::value);
#if defined(LIBIMP_CPP_17) && defined(__cpp_lib_memory_resource)
  EXPECT_TRUE (ipc::mem::has_deallocate<std::ipc::mem::memory_resource>::value);
  EXPECT_FALSE(ipc::mem::has_deallocate<std::ipc::mem::polymorphic_allocator<int>>::value);
#endif
}

TEST(allocator, construct_copy_move) {
  ipc::mem::new_delete_resource mem_res;
  dummy_resource dummy_res;
  ipc::mem::allocator alc1{&mem_res}, alc2{&dummy_res};
  auto p = alc1.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc1.deallocate(p, 128));
  ASSERT_EQ(alc2.allocate(128), nullptr);

  ipc::mem::allocator alc3{alc1}, alc4{alc2}, alc5{std::move(alc1)};

  p = alc3.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc3.deallocate(p, 128));

  ASSERT_EQ(alc4.allocate(128), nullptr);

  p = alc5.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc5.deallocate(p, 128));
}

TEST(allocator, swap) {
  ipc::mem::new_delete_resource mem_res;
  dummy_resource dummy_res;
  ipc::mem::allocator alc1{&mem_res}, alc2{&dummy_res};
  alc1.swap(alc2);
  auto p = alc2.allocate(128);
  ASSERT_NE(p, nullptr);
  ASSERT_NO_THROW(alc2.deallocate(p, 128));
  ASSERT_EQ(alc1.allocate(128), nullptr);
}

TEST(allocator, invalid_alloc_free) {
  ipc::mem::allocator alc1;
  EXPECT_EQ(alc1.allocate(0), nullptr);
  EXPECT_NO_THROW(alc1.deallocate(nullptr, 128));
  EXPECT_NO_THROW(alc1.deallocate(nullptr, 0));
  EXPECT_NO_THROW(alc1.deallocate(&alc1, 0));
}

TEST(allocator, sizeof) {
  EXPECT_EQ(sizeof(ipc::mem::allocator), sizeof(void *) * 2);
}

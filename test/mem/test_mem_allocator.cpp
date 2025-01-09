
#include <vector>
#include <utility>

#include "test.h"

#include "libipc/mem/allocator.h"

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

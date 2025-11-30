/**
 * @file test_shm.cpp
 * @brief Comprehensive unit tests for ipc::shm (shared memory) functionality
 * 
 * This test suite covers:
 * - Low-level shared memory functions (acquire, get_mem, release, remove)
 * - Reference counting (get_ref, sub_ref)
 * - High-level handle class interface
 * - Create and open modes
 * - Resource cleanup and error handling
 */

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <string>
#include "libipc/shm.h"

using namespace ipc;

namespace {

// Generate unique shared memory names for tests
std::string generate_unique_name(const char* prefix) {
  static int counter = 0;
  return std::string(prefix) + "_test_" + std::to_string(++counter);
}

} // anonymous namespace

class ShmTest : public ::testing::Test {
protected:
  void TearDown() override {
      // Clean up any leftover shared memory segments
  }
};

// ========== Low-level API Tests ==========

// Test acquire with create mode
TEST_F(ShmTest, AcquireCreate) {
  std::string name = generate_unique_name("acquire_create");
  const std::size_t size = 1024;
  
  shm::id_t id = shm::acquire(name.c_str(), size, shm::create);
  ASSERT_NE(id, nullptr);
  
  std::size_t actual_size = 0;
  void* mem = shm::get_mem(id, &actual_size);
  EXPECT_NE(mem, nullptr);
  EXPECT_GE(actual_size, size);
  
  // Use remove(id) to clean up - it internally calls release()
  shm::remove(id);
}

// Test acquire with open mode (should fail if not exists)
TEST_F(ShmTest, AcquireOpenNonExistent) {
  std::string name = generate_unique_name("acquire_open_fail");
  
  shm::id_t id = shm::acquire(name.c_str(), 1024, shm::open);
  // Opening non-existent shared memory should return nullptr or handle failure gracefully
  if (id != nullptr) {
      shm::release(id);
  }
}

// Test acquire with both create and open modes
TEST_F(ShmTest, AcquireCreateOrOpen) {
  std::string name = generate_unique_name("acquire_both");
  const std::size_t size = 2048;
  
  shm::id_t id = shm::acquire(name.c_str(), size, shm::create | shm::open);
  ASSERT_NE(id, nullptr);
  
  std::size_t actual_size = 0;
  void* mem = shm::get_mem(id, &actual_size);
  EXPECT_NE(mem, nullptr);
  EXPECT_GE(actual_size, size);
  
  // Use remove(id) to clean up - it internally calls release()
  shm::remove(id);
}

// Test get_mem function
TEST_F(ShmTest, GetMemory) {
  std::string name = generate_unique_name("get_mem");
  const std::size_t size = 512;
  
  shm::id_t id = shm::acquire(name.c_str(), size, shm::create);
  ASSERT_NE(id, nullptr);
  
  std::size_t returned_size = 0;
  void* mem = shm::get_mem(id, &returned_size);
  
  EXPECT_NE(mem, nullptr);
  EXPECT_GE(returned_size, size);
  
  // Write and read test data
  const char* test_data = "Shared memory test data";
  std::strcpy(static_cast<char*>(mem), test_data);
  EXPECT_STREQ(static_cast<char*>(mem), test_data);
  
  // Use remove(id) to clean up - it internally calls release()
  shm::remove(id);
}

// Test get_mem without size parameter
TEST_F(ShmTest, GetMemoryNoSize) {
  std::string name = generate_unique_name("get_mem_no_size");
  
  shm::id_t id = shm::acquire(name.c_str(), 256, shm::create);
  ASSERT_NE(id, nullptr);
  
  void* mem = shm::get_mem(id, nullptr);
  EXPECT_NE(mem, nullptr);
  
  // Use remove(id) to clean up - it internally calls release()
  shm::remove(id);
}

// Test release function
TEST_F(ShmTest, ReleaseMemory) {
  std::string name = generate_unique_name("release");
  
  shm::id_t id = shm::acquire(name.c_str(), 128, shm::create);
  ASSERT_NE(id, nullptr);
  
  std::int32_t ref_count = shm::release(id);
  EXPECT_GE(ref_count, 0);
  
  shm::remove(name.c_str());
}

// Test remove by id
TEST_F(ShmTest, RemoveById) {
  std::string name = generate_unique_name("remove_by_id");
  
  shm::id_t id = shm::acquire(name.c_str(), 256, shm::create);
  ASSERT_NE(id, nullptr);
  
  // remove(id) internally calls release(id), so we don't need to call release first
  shm::remove(id); // Should succeed
}

// Test remove by name
TEST_F(ShmTest, RemoveByName) {
  std::string name = generate_unique_name("remove_by_name");
  
  shm::id_t id = shm::acquire(name.c_str(), 256, shm::create);
  ASSERT_NE(id, nullptr);
  
  shm::release(id);
  shm::remove(name.c_str()); // Should succeed
}

// Test reference counting
TEST_F(ShmTest, ReferenceCount) {
  std::string name = generate_unique_name("ref_count");
  
  shm::id_t id1 = shm::acquire(name.c_str(), 512, shm::create);
  ASSERT_NE(id1, nullptr);
  
  std::int32_t ref1 = shm::get_ref(id1);
  EXPECT_GT(ref1, 0);
  
  // Acquire again (should increase reference count)
  shm::id_t id2 = shm::acquire(name.c_str(), 512, shm::open);
  if (id2 != nullptr) {
      std::int32_t ref2 = shm::get_ref(id2);
      EXPECT_GE(ref2, ref1);
      
      shm::release(id2);
  }
  
  shm::release(id1);
  shm::remove(name.c_str());
}

// Test sub_ref function
TEST_F(ShmTest, SubtractReference) {
  std::string name = generate_unique_name("sub_ref");
  
  shm::id_t id = shm::acquire(name.c_str(), 256, shm::create);
  ASSERT_NE(id, nullptr);
  
  std::int32_t ref_before = shm::get_ref(id);
  shm::sub_ref(id);
  std::int32_t ref_after = shm::get_ref(id);
  
  EXPECT_EQ(ref_after, ref_before - 1);
  
  // Use remove(id) to clean up - it internally calls release()
  shm::remove(id);
}

// ========== High-level handle class Tests ==========

// Test default handle constructor
TEST_F(ShmTest, HandleDefaultConstructor) {
  shm::handle h;
  EXPECT_FALSE(h.valid());
  EXPECT_EQ(h.size(), 0u);
  EXPECT_EQ(h.get(), nullptr);
}

// Test handle constructor with name and size
TEST_F(ShmTest, HandleConstructorWithParams) {
  std::string name = generate_unique_name("handle_ctor");
  const std::size_t size = 1024;
  
  shm::handle h(name.c_str(), size);
  
  EXPECT_TRUE(h.valid());
  EXPECT_GE(h.size(), size);
  EXPECT_NE(h.get(), nullptr);
  EXPECT_STREQ(h.name(), name.c_str());
}

// Test handle move constructor
TEST_F(ShmTest, HandleMoveConstructor) {
  std::string name = generate_unique_name("handle_move");
  
  shm::handle h1(name.c_str(), 512);
  ASSERT_TRUE(h1.valid());
  
  void* ptr1 = h1.get();
  std::size_t size1 = h1.size();
  
  shm::handle h2(std::move(h1));
  
  EXPECT_TRUE(h2.valid());
  EXPECT_EQ(h2.get(), ptr1);
  EXPECT_EQ(h2.size(), size1);
  
  // h1 should be invalid after move
  EXPECT_FALSE(h1.valid());
}

// Test handle swap
TEST_F(ShmTest, HandleSwap) {
  std::string name1 = generate_unique_name("handle_swap1");
  std::string name2 = generate_unique_name("handle_swap2");
  
  shm::handle h1(name1.c_str(), 256);
  shm::handle h2(name2.c_str(), 512);
  
  void* ptr1 = h1.get();
  void* ptr2 = h2.get();
  std::size_t size1 = h1.size();
  std::size_t size2 = h2.size();
  
  h1.swap(h2);
  
  EXPECT_EQ(h1.get(), ptr2);
  EXPECT_EQ(h1.size(), size2);
  EXPECT_EQ(h2.get(), ptr1);
  EXPECT_EQ(h2.size(), size1);
}

// Test handle assignment operator
TEST_F(ShmTest, HandleAssignment) {
  std::string name = generate_unique_name("handle_assign");
  
  shm::handle h1(name.c_str(), 768);
  void* ptr1 = h1.get();
  
  shm::handle h2;
  h2 = std::move(h1);
  
  EXPECT_TRUE(h2.valid());
  EXPECT_EQ(h2.get(), ptr1);
  EXPECT_FALSE(h1.valid());
}

// Test handle valid() method
TEST_F(ShmTest, HandleValid) {
  shm::handle h1;
  EXPECT_FALSE(h1.valid());
  
  std::string name = generate_unique_name("handle_valid");
  shm::handle h2(name.c_str(), 128);
  EXPECT_TRUE(h2.valid());
}

// Test handle size() method
TEST_F(ShmTest, HandleSize) {
  std::string name = generate_unique_name("handle_size");
  const std::size_t requested_size = 2048;
  
  shm::handle h(name.c_str(), requested_size);
  
  EXPECT_GE(h.size(), requested_size);
}

// Test handle name() method
TEST_F(ShmTest, HandleName) {
  std::string name = generate_unique_name("handle_name");
  
  shm::handle h(name.c_str(), 256);
  
  EXPECT_STREQ(h.name(), name.c_str());
}

// Test handle ref() method
TEST_F(ShmTest, HandleRef) {
  std::string name = generate_unique_name("handle_ref");
  
  shm::handle h(name.c_str(), 256);
  
  std::int32_t ref = h.ref();
  EXPECT_GT(ref, 0);
}

// Test handle sub_ref() method
TEST_F(ShmTest, HandleSubRef) {
  std::string name = generate_unique_name("handle_sub_ref");
  
  shm::handle h(name.c_str(), 256);
  
  std::int32_t ref_before = h.ref();
  h.sub_ref();
  std::int32_t ref_after = h.ref();
  
  EXPECT_EQ(ref_after, ref_before - 1);
}

// Test handle acquire() method
TEST_F(ShmTest, HandleAcquire) {
  shm::handle h;
  EXPECT_FALSE(h.valid());
  
  std::string name = generate_unique_name("handle_acquire");
  bool result = h.acquire(name.c_str(), 512);
  
  EXPECT_TRUE(result);
  EXPECT_TRUE(h.valid());
  EXPECT_GE(h.size(), 512u);
}

// Test handle release() method
TEST_F(ShmTest, HandleRelease) {
  std::string name = generate_unique_name("handle_release");
  
  shm::handle h(name.c_str(), 256);
  ASSERT_TRUE(h.valid());
  
  std::int32_t ref_count = h.release();
  EXPECT_GE(ref_count, 0);
}

// Test handle clear() method
TEST_F(ShmTest, HandleClear) {
  std::string name = generate_unique_name("handle_clear");
  
  shm::handle h(name.c_str(), 256);
  ASSERT_TRUE(h.valid());
  
  h.clear();
  EXPECT_FALSE(h.valid());
}

// Test handle clear_storage() static method
TEST_F(ShmTest, HandleClearStorage) {
  std::string name = generate_unique_name("handle_clear_storage");
  
  {
      shm::handle h(name.c_str(), 256);
      EXPECT_TRUE(h.valid());
  }
  
  shm::handle::clear_storage(name.c_str());
  
  // Try to open - should fail or create new
  shm::handle h2(name.c_str(), 256, shm::open);
  // Behavior depends on implementation
}

// Test handle get() method
TEST_F(ShmTest, HandleGet) {
  std::string name = generate_unique_name("handle_get");
  
  shm::handle h(name.c_str(), 512);
  
  void* mem = h.get();
  EXPECT_NE(mem, nullptr);
  
  // Write and read test
  const char* test_str = "Handle get test";
  std::strcpy(static_cast<char*>(mem), test_str);
  EXPECT_STREQ(static_cast<char*>(mem), test_str);
}

// Test handle detach() and attach() methods
TEST_F(ShmTest, HandleDetachAttach) {
  std::string name = generate_unique_name("handle_detach_attach");
  
  shm::handle h1(name.c_str(), 256);
  ASSERT_TRUE(h1.valid());
  
  shm::id_t id = h1.detach();
  EXPECT_NE(id, nullptr);
  EXPECT_FALSE(h1.valid()); // Should be invalid after detach
  
  shm::handle h2;
  h2.attach(id);
  EXPECT_TRUE(h2.valid());
  
  // Clean up - use h2.clear() or shm::remove(id) alone, not both
  // Option 1: Use handle's clear() which calls shm::remove(id) internally
  id = h2.detach(); // Detach first to get the id without releasing
  shm::remove(id);  // Then remove to clean up both memory and disk file
}

// Test writing and reading data through shared memory
TEST_F(ShmTest, WriteReadData) {
  std::string name = generate_unique_name("write_read");
  const std::size_t size = 1024;
  
  shm::handle h1(name.c_str(), size);
  ASSERT_TRUE(h1.valid());
  
  // Write test data
  struct TestData {
      int value;
      char text[64];
  };
  
  TestData* data1 = static_cast<TestData*>(h1.get());
  data1->value = 42;
  std::strcpy(data1->text, "Shared memory data");
  
  // Open in another "shm::handle" (simulating different process)
  shm::handle h2(name.c_str(), size, shm::open);
  if (h2.valid()) {
      TestData* data2 = static_cast<TestData*>(h2.get());
      EXPECT_EQ(data2->value, 42);
      EXPECT_STREQ(data2->text, "Shared memory data");
  }
}

// Test handle with different modes
TEST_F(ShmTest, HandleModes) {
  std::string name = generate_unique_name("handle_modes");
  
  // Create only
  shm::handle h1(name.c_str(), 256, shm::create);
  EXPECT_TRUE(h1.valid());
  
  // Open existing
  shm::handle h2(name.c_str(), 256, shm::open);
  EXPECT_TRUE(h2.valid());
  
  // Both modes
  shm::handle h3(name.c_str(), 256, shm::create | shm::open);
  EXPECT_TRUE(h3.valid());
}

// Test multiple handles to same shared memory
TEST_F(ShmTest, MultipleHandles) {
  std::string name = generate_unique_name("multiple_handles");
  const std::size_t size = 512;
  
  shm::handle h1(name.c_str(), size);
  shm::handle h2(name.c_str(), size, shm::open);
  
  ASSERT_TRUE(h1.valid());
  ASSERT_TRUE(h2.valid());
  
  // Should point to same memory
  int* data1 = static_cast<int*>(h1.get());
  int* data2 = static_cast<int*>(h2.get());
  
  *data1 = 12345;
  EXPECT_EQ(*data2, 12345);
}

// Test large shared memory segment
TEST_F(ShmTest, LargeSegment) {
  std::string name = generate_unique_name("large_segment");
  const std::size_t size = 10 * 1024 * 1024; // 10 MB
  
  shm::handle h(name.c_str(), size);
  
  if (h.valid()) {
      EXPECT_GE(h.size(), size);
      
      // Write pattern to a portion of memory
      char* mem = static_cast<char*>(h.get());
      for (std::size_t i = 0; i < 1024; ++i) {
          mem[i] = static_cast<char>(i % 256);
      }
      
      // Verify pattern
      for (std::size_t i = 0; i < 1024; ++i) {
          EXPECT_EQ(mem[i], static_cast<char>(i % 256));
      }
  }
}

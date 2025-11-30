/**
 * @file test_buffer.cpp
 * @brief Comprehensive unit tests for ipc::buffer class
 * 
 * This test suite covers all public interfaces of the buffer class including:
 * - Constructors (default, with pointer and destructor, from array, from char)
 * - Move semantics
 * - Copy operations through assignment
 * - Basic operations (empty, data, size)
 * - Conversion methods (to_tuple, to_vector, get<T>)
 * - Comparison operators
 */

#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "libipc/buffer.h"

using namespace ipc;

namespace {

// Custom destructor tracker for testing
struct DestructorTracker {
  static int count;
  static void reset() { count = 0; }
  static void destructor(void* p, std::size_t) {
      ++count;
      delete[] static_cast<char*>(p);
  }
};
int DestructorTracker::count = 0;

} // anonymous namespace

class BufferTest : public ::testing::Test {
protected:
  void SetUp() override {
      DestructorTracker::reset();
  }
};

// Test default constructor
TEST_F(BufferTest, DefaultConstructor) {
  buffer buf;
  EXPECT_TRUE(buf.empty());
  EXPECT_EQ(buf.size(), 0u);
  EXPECT_EQ(buf.data(), nullptr);
}

// Test constructor with pointer, size, and destructor
TEST_F(BufferTest, ConstructorWithDestructor) {
  const char* test_data = "Hello, World!";
  std::size_t size = std::strlen(test_data) + 1;
  char* data = new char[size];
  std::strcpy(data, test_data);
  
  buffer buf(data, size, DestructorTracker::destructor);
  
  EXPECT_FALSE(buf.empty());
  EXPECT_EQ(buf.size(), size);
  EXPECT_NE(buf.data(), nullptr);
  EXPECT_STREQ(static_cast<const char*>(buf.data()), test_data);
}

// Test destructor is called
TEST_F(BufferTest, DestructorCalled) {
  {
      char* data = new char[100];
      buffer buf(data, 100, DestructorTracker::destructor);
      EXPECT_EQ(DestructorTracker::count, 0);
  }
  EXPECT_EQ(DestructorTracker::count, 1);
}

// Test constructor with mem_to_free parameter
// Scenario: allocate a large block, but only use a portion as data
TEST_F(BufferTest, ConstructorWithMemToFree) {
  // Allocate a block of 100 bytes
  char* allocated_block = new char[100];
  
  // But only use the middle 50 bytes as data (offset 25)
  char* data_start = allocated_block + 25;
  std::strcpy(data_start, "Offset data");
  
  // When destroyed, should free the entire allocated_block, not just data_start
  buffer buf(data_start, 50, DestructorTracker::destructor, allocated_block);
  
  EXPECT_FALSE(buf.empty());
  EXPECT_EQ(buf.size(), 50u);
  EXPECT_EQ(buf.data(), data_start);
  EXPECT_STREQ(static_cast<const char*>(buf.data()), "Offset data");
  
  // Destructor will be called with allocated_block (not data_start)
  // This correctly frees the entire allocation
}

// Test constructor without destructor
TEST_F(BufferTest, ConstructorWithoutDestructor) {
  char stack_data[20] = "Stack data";
  
  buffer buf(stack_data, 20);
  
  EXPECT_FALSE(buf.empty());
  EXPECT_EQ(buf.size(), 20u);
  EXPECT_EQ(buf.data(), stack_data);
}

// Test constructor from byte array
TEST_F(BufferTest, ConstructorFromByteArray) {
  byte_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  
  buffer buf(data);
  
  EXPECT_FALSE(buf.empty());
  EXPECT_EQ(buf.size(), 10u);
  
  const byte_t* buf_data = buf.get<const byte_t*>();
  for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(buf_data[i], i);
  }
}

// Test constructor from single char
TEST_F(BufferTest, ConstructorFromChar) {
  char c = 'X';
  
  buffer buf(c);
  
  EXPECT_FALSE(buf.empty());
  EXPECT_EQ(buf.size(), sizeof(char));
  EXPECT_EQ(*buf.get<const char*>(), 'X');
}

// Test move constructor
TEST_F(BufferTest, MoveConstructor) {
  char* data = new char[30];
  std::strcpy(data, "Move test");
  
  buffer buf1(data, 30, DestructorTracker::destructor);
  void* original_ptr = buf1.data();
  std::size_t original_size = buf1.size();
  
  buffer buf2(std::move(buf1));
  
  // buf2 should have the original data
  EXPECT_EQ(buf2.data(), original_ptr);
  EXPECT_EQ(buf2.size(), original_size);
  EXPECT_FALSE(buf2.empty());
  
  // buf1 should be empty after move
  EXPECT_TRUE(buf1.empty());
  EXPECT_EQ(buf1.size(), 0u);
}

// Test swap
TEST_F(BufferTest, Swap) {
  char* data1 = new char[20];
  char* data2 = new char[30];
  std::strcpy(data1, "Buffer 1");
  std::strcpy(data2, "Buffer 2");
  
  buffer buf1(data1, 20, DestructorTracker::destructor);
  buffer buf2(data2, 30, DestructorTracker::destructor);
  
  void* ptr1 = buf1.data();
  void* ptr2 = buf2.data();
  std::size_t size1 = buf1.size();
  std::size_t size2 = buf2.size();
  
  buf1.swap(buf2);
  
  EXPECT_EQ(buf1.data(), ptr2);
  EXPECT_EQ(buf1.size(), size2);
  EXPECT_EQ(buf2.data(), ptr1);
  EXPECT_EQ(buf2.size(), size1);
}

// Test assignment operator (move semantics)
TEST_F(BufferTest, AssignmentOperator) {
  char* data = new char[40];
  std::strcpy(data, "Assignment test");
  
  buffer buf1(data, 40, DestructorTracker::destructor);
  void* original_ptr = buf1.data();
  
  buffer buf2;
  buf2 = std::move(buf1);
  
  EXPECT_EQ(buf2.data(), original_ptr);
  EXPECT_FALSE(buf2.empty());
}

// Test empty() method
TEST_F(BufferTest, EmptyMethod) {
  buffer buf1;
  EXPECT_TRUE(buf1.empty());
  
  char* data = new char[10];
  buffer buf2(data, 10, DestructorTracker::destructor);
  EXPECT_FALSE(buf2.empty());
}

// Test data() const method
TEST_F(BufferTest, DataConstMethod) {
  const char* test_str = "Const data test";
  std::size_t size = std::strlen(test_str) + 1;
  char* data = new char[size];
  std::strcpy(data, test_str);
  
  const buffer buf(data, size, DestructorTracker::destructor);
  
  const void* const_data = buf.data();
  EXPECT_NE(const_data, nullptr);
  EXPECT_STREQ(static_cast<const char*>(const_data), test_str);
}

// Test get<T>() template method
TEST_F(BufferTest, GetTemplateMethod) {
  int* int_data = new int[5]{1, 2, 3, 4, 5};
  
  buffer buf(int_data, 5 * sizeof(int), [](void* p, std::size_t) {
      delete[] static_cast<int*>(p);
  });
  
  int* retrieved = buf.get<int*>();
  EXPECT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved[0], 1);
  EXPECT_EQ(retrieved[4], 5);
}

// Test to_tuple() non-const version
TEST_F(BufferTest, ToTupleNonConst) {
  char* data = new char[25];
  std::strcpy(data, "Tuple test");
  
  buffer buf(data, 25, DestructorTracker::destructor);
  
  auto [ptr, size] = buf.to_tuple();
  EXPECT_EQ(ptr, buf.data());
  EXPECT_EQ(size, buf.size());
  EXPECT_EQ(size, 25u);
}

// Test to_tuple() const version
TEST_F(BufferTest, ToTupleConst) {
  char* data = new char[30];
  std::strcpy(data, "Const tuple");
  
  const buffer buf(data, 30, DestructorTracker::destructor);
  
  auto [ptr, size] = buf.to_tuple();
  EXPECT_EQ(ptr, buf.data());
  EXPECT_EQ(size, buf.size());
  EXPECT_EQ(size, 30u);
}

// Test to_vector() method
TEST_F(BufferTest, ToVector) {
  byte_t data_arr[5] = {10, 20, 30, 40, 50};
  
  buffer buf(data_arr, 5);
  
  std::vector<byte_t> vec = buf.to_vector();
  ASSERT_EQ(vec.size(), 5u);
  EXPECT_EQ(vec[0], 10);
  EXPECT_EQ(vec[1], 20);
  EXPECT_EQ(vec[2], 30);
  EXPECT_EQ(vec[3], 40);
  EXPECT_EQ(vec[4], 50);
}

// Test equality operator
TEST_F(BufferTest, EqualityOperator) {
  byte_t data1[5] = {1, 2, 3, 4, 5};
  byte_t data2[5] = {1, 2, 3, 4, 5};
  byte_t data3[5] = {5, 4, 3, 2, 1};
  
  buffer buf1(data1, 5);
  buffer buf2(data2, 5);
  buffer buf3(data3, 5);
  
  EXPECT_TRUE(buf1 == buf2);
  EXPECT_FALSE(buf1 == buf3);
}

// Test inequality operator
TEST_F(BufferTest, InequalityOperator) {
  byte_t data1[5] = {1, 2, 3, 4, 5};
  byte_t data2[5] = {1, 2, 3, 4, 5};
  byte_t data3[5] = {5, 4, 3, 2, 1};
  
  buffer buf1(data1, 5);
  buffer buf2(data2, 5);
  buffer buf3(data3, 5);
  
  EXPECT_FALSE(buf1 != buf2);
  EXPECT_TRUE(buf1 != buf3);
}

// Test size mismatch in equality
TEST_F(BufferTest, EqualityWithDifferentSizes) {
  byte_t data1[5] = {1, 2, 3, 4, 5};
  byte_t data2[3] = {1, 2, 3};
  
  buffer buf1(data1, 5);
  buffer buf2(data2, 3);
  
  EXPECT_FALSE(buf1 == buf2);
  EXPECT_TRUE(buf1 != buf2);
}

// Test empty buffers comparison
TEST_F(BufferTest, EmptyBuffersComparison) {
  buffer buf1;
  buffer buf2;
  
  EXPECT_TRUE(buf1 == buf2);
  EXPECT_FALSE(buf1 != buf2);
}

// Test large buffer
TEST_F(BufferTest, LargeBuffer) {
  const std::size_t large_size = 1024 * 1024; // 1MB
  char* large_data = new char[large_size];
  
  // Fill with pattern
  for (std::size_t i = 0; i < large_size; ++i) {
      large_data[i] = static_cast<char>(i % 256);
  }
  
  buffer buf(large_data, large_size, [](void* p, std::size_t) {
      delete[] static_cast<char*>(p);
  });
  
  EXPECT_FALSE(buf.empty());
  EXPECT_EQ(buf.size(), large_size);
  
  // Verify pattern
  const char* data_ptr = buf.get<const char*>();
  for (std::size_t i = 0; i < 100; ++i) { // Check first 100 bytes
      EXPECT_EQ(data_ptr[i], static_cast<char>(i % 256));
  }
}

// Test multiple move operations
TEST_F(BufferTest, MultipleMoves) {
  char* data = new char[15];
  std::strcpy(data, "Multi-move");
  void* original_ptr = data;
  
  buffer buf1(data, 15, DestructorTracker::destructor);
  buffer buf2(std::move(buf1));
  buffer buf3(std::move(buf2));
  buffer buf4(std::move(buf3));
  
  EXPECT_EQ(buf4.data(), original_ptr);
  EXPECT_TRUE(buf1.empty());
  EXPECT_TRUE(buf2.empty());
  EXPECT_TRUE(buf3.empty());
  EXPECT_FALSE(buf4.empty());
}

// Test self-assignment safety
TEST_F(BufferTest, SelfAssignment) {
  char* data = new char[20];
  std::strcpy(data, "Self-assign");
  
  buffer buf(data, 20, DestructorTracker::destructor);
  void* original_ptr = buf.data();
  std::size_t original_size = buf.size();
  
  buf = std::move(buf); // Self-assignment
  
  // Should remain valid
  EXPECT_EQ(buf.data(), original_ptr);
  EXPECT_EQ(buf.size(), original_size);
}

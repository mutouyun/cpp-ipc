/**
 * @file test_locks.cpp
 * @brief Comprehensive unit tests for ipc::rw_lock and ipc::spin_lock classes
 * 
 * This test suite covers:
 * - spin_lock: basic lock/unlock operations
 * - rw_lock: read-write lock functionality
 * - rw_lock: exclusive (write) locks
 * - rw_lock: shared (read) locks
 * - Concurrent access patterns
 * - Reader-writer scenarios
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "libipc/rw_lock.h"

using namespace ipc;

// ========== spin_lock Tests ==========

class SpinLockTest : public ::testing::Test {
protected:
  void TearDown() override {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
};

// Test basic lock and unlock
TEST_F(SpinLockTest, BasicLockUnlock) {
  spin_lock lock;
  
  lock.lock();
  lock.unlock();
  
  // Should complete without hanging
}

// Test multiple lock/unlock cycles
TEST_F(SpinLockTest, MultipleCycles) {
  spin_lock lock;
  
  for (int i = 0; i < 100; ++i) {
      lock.lock();
      lock.unlock();
  }
}

// Test critical section protection
TEST_F(SpinLockTest, CriticalSection) {
  spin_lock lock;
  int counter = 0;
  const int iterations = 1000;
  
  auto increment_task = [&]() {
      for (int i = 0; i < iterations; ++i) {
          lock.lock();
          ++counter;
          lock.unlock();
      }
  };
  
  std::thread t1(increment_task);
  std::thread t2(increment_task);
  
  t1.join();
  t2.join();
  
  EXPECT_EQ(counter, iterations * 2);
}

// Test mutual exclusion
TEST_F(SpinLockTest, MutualExclusion) {
  spin_lock lock;
  std::atomic<bool> thread1_in_cs{false};
  std::atomic<bool> thread2_in_cs{false};
  std::atomic<bool> violation{false};
  
  auto cs_task = [&](std::atomic<bool>& my_flag, std::atomic<bool>& other_flag) {
      for (int i = 0; i < 100; ++i) {
          lock.lock();
          
          my_flag.store(true);
          if (other_flag.load()) {
              violation.store(true);
          }
          
          std::this_thread::sleep_for(std::chrono::microseconds(10));
          
          my_flag.store(false);
          lock.unlock();
          
          std::this_thread::yield();
      }
  };
  
  std::thread t1(cs_task, std::ref(thread1_in_cs), std::ref(thread2_in_cs));
  std::thread t2(cs_task, std::ref(thread2_in_cs), std::ref(thread1_in_cs));
  
  t1.join();
  t2.join();
  
  EXPECT_FALSE(violation.load());
}

// Test concurrent access
TEST_F(SpinLockTest, ConcurrentAccess) {
  spin_lock lock;
  std::atomic<int> shared_data{0};
  const int num_threads = 4;
  const int ops_per_thread = 100;
  
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&]() {
          for (int j = 0; j < ops_per_thread; ++j) {
              lock.lock();
              int temp = shared_data.load();
              std::this_thread::yield();
              shared_data.store(temp + 1);
              lock.unlock();
          }
      });
  }
  
  for (auto& t : threads) {
      t.join();
  }
  
  EXPECT_EQ(shared_data.load(), num_threads * ops_per_thread);
}

// Test rapid lock/unlock
TEST_F(SpinLockTest, RapidLockUnlock) {
  spin_lock lock;
  
  auto rapid_task = [&]() {
      for (int i = 0; i < 10000; ++i) {
          lock.lock();
          lock.unlock();
      }
  };
  
  std::thread t1(rapid_task);
  std::thread t2(rapid_task);
  
  t1.join();
  t2.join();
  
  // Should complete without deadlock
}

// Test contention scenario
TEST_F(SpinLockTest, Contention) {
  spin_lock lock;
  std::atomic<int> work_done{0};
  const int num_threads = 8;
  
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&]() {
          for (int j = 0; j < 50; ++j) {
              lock.lock();
              ++work_done;
              std::this_thread::sleep_for(std::chrono::microseconds(100));
              lock.unlock();
              std::this_thread::yield();
          }
      });
  }
  
  for (auto& t : threads) {
      t.join();
  }
  
  EXPECT_EQ(work_done.load(), num_threads * 50);
}

// ========== rw_lock Tests ==========

class RWLockTest : public ::testing::Test {
protected:
  void TearDown() override {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
};

// Test basic write lock and unlock
TEST_F(RWLockTest, BasicWriteLock) {
  rw_lock lock;
  
  lock.lock();
  lock.unlock();
  
  // Should complete without hanging
}

// Test basic read lock and unlock
TEST_F(RWLockTest, BasicReadLock) {
  rw_lock lock;
  
  lock.lock_shared();
  lock.unlock_shared();
  
  // Should complete without hanging
}

// Test multiple write cycles
TEST_F(RWLockTest, MultipleWriteCycles) {
  rw_lock lock;
  
  for (int i = 0; i < 100; ++i) {
      lock.lock();
      lock.unlock();
  }
}

// Test multiple read cycles
TEST_F(RWLockTest, MultipleReadCycles) {
  rw_lock lock;
  
  for (int i = 0; i < 100; ++i) {
      lock.lock_shared();
      lock.unlock_shared();
  }
}

// Test write lock protects data
TEST_F(RWLockTest, WriteLockProtection) {
  rw_lock lock;
  int data = 0;
  const int iterations = 500;
  
  auto writer_task = [&]() {
      for (int i = 0; i < iterations; ++i) {
          lock.lock();
          ++data;
          lock.unlock();
      }
  };
  
  std::thread t1(writer_task);
  std::thread t2(writer_task);
  
  t1.join();
  t2.join();
  
  EXPECT_EQ(data, iterations * 2);
}

// Test multiple readers can access concurrently
TEST_F(RWLockTest, ConcurrentReaders) {
  rw_lock lock;
  std::atomic<int> concurrent_readers{0};
  std::atomic<int> max_concurrent{0};
  
  const int num_readers = 5;
  
  std::vector<std::thread> readers;
  for (int i = 0; i < num_readers; ++i) {
      readers.emplace_back([&]() {
          for (int j = 0; j < 20; ++j) {
              lock.lock_shared();
              
              int current = ++concurrent_readers;
              
              // Track maximum concurrent readers
              int current_max = max_concurrent.load();
              while (current > current_max) {
                  if (max_concurrent.compare_exchange_weak(current_max, current)) {
                      break;
                  }
              }
              
              std::this_thread::sleep_for(std::chrono::microseconds(100));
              
              --concurrent_readers;
              lock.unlock_shared();
              
              std::this_thread::yield();
          }
      });
  }
  
  for (auto& t : readers) {
      t.join();
  }
  
  // Should have had multiple concurrent readers
  EXPECT_GT(max_concurrent.load(), 1);
}

// Test writers have exclusive access
TEST_F(RWLockTest, WriterExclusiveAccess) {
  rw_lock lock;
  std::atomic<bool> writer_in_cs{false};
  std::atomic<bool> violation{false};
  
  auto writer_task = [&]() {
      for (int i = 0; i < 50; ++i) {
          lock.lock();
          
          if (writer_in_cs.exchange(true)) {
              violation.store(true);
          }
          
          std::this_thread::sleep_for(std::chrono::microseconds(50));
          
          writer_in_cs.store(false);
          lock.unlock();
          
          std::this_thread::yield();
      }
  };
  
  std::thread t1(writer_task);
  std::thread t2(writer_task);
  
  t1.join();
  t2.join();
  
  EXPECT_FALSE(violation.load());
}

// Test readers and writers don't overlap
TEST_F(RWLockTest, ReadersWritersNoOverlap) {
  rw_lock lock;
  std::atomic<int> readers{0};
  std::atomic<bool> writer_active{false};
  std::atomic<bool> violation{false};
  
  auto reader_task = [&]() {
      for (int i = 0; i < 30; ++i) {
          lock.lock_shared();
          
          ++readers;
          if (writer_active.load()) {
              violation.store(true);
          }
          
          std::this_thread::sleep_for(std::chrono::microseconds(50));
          
          --readers;
          lock.unlock_shared();
          
          std::this_thread::yield();
      }
  };
  
  auto writer_task = [&]() {
      for (int i = 0; i < 15; ++i) {
          lock.lock();
          
          writer_active.store(true);
          if (readers.load() > 0) {
              violation.store(true);
          }
          
          std::this_thread::sleep_for(std::chrono::microseconds(50));
          
          writer_active.store(false);
          lock.unlock();
          
          std::this_thread::yield();
      }
  };
  
  std::thread r1(reader_task);
  std::thread r2(reader_task);
  std::thread w1(writer_task);
  
  r1.join();
  r2.join();
  w1.join();
  
  EXPECT_FALSE(violation.load());
}

// Test read-write-read pattern
TEST_F(RWLockTest, ReadWriteReadPattern) {
  rw_lock lock;
  int data = 0;
  
  auto pattern_task = [&](int id) {
      for (int i = 0; i < 20; ++i) {
          // Read
          lock.lock_shared();
          int read_val = data;
          lock.unlock_shared();
          
          std::this_thread::yield();
          
          // Write
          lock.lock();
          data = read_val + 1;
          lock.unlock();
          
          std::this_thread::yield();
      }
  };
  
  std::thread t1(pattern_task, 1);
  std::thread t2(pattern_task, 2);
  
  t1.join();
  t2.join();
  
  EXPECT_EQ(data, 40);
}

// Test many readers, one writer
TEST_F(RWLockTest, ManyReadersOneWriter) {
  rw_lock lock;
  std::atomic<int> data{0};
  std::atomic<int> read_count{0};
  
  const int num_readers = 10;
  
  std::vector<std::thread> readers;
  for (int i = 0; i < num_readers; ++i) {
      readers.emplace_back([&]() {
          for (int j = 0; j < 50; ++j) {
              lock.lock_shared();
              int val = data.load();
              ++read_count;
              lock.unlock_shared();
              std::this_thread::yield();
          }
      });
  }
  
  std::thread writer([&]() {
      for (int i = 0; i < 100; ++i) {
          lock.lock();
          data.store(data.load() + 1);
          lock.unlock();
          std::this_thread::yield();
      }
  });
  
  for (auto& t : readers) {
      t.join();
  }
  writer.join();
  
  EXPECT_EQ(data.load(), 100);
  EXPECT_EQ(read_count.load(), num_readers * 50);
}

// Test rapid read lock/unlock
TEST_F(RWLockTest, RapidReadLocks) {
  rw_lock lock;
  
  auto rapid_read = [&]() {
      for (int i = 0; i < 5000; ++i) {
          lock.lock_shared();
          lock.unlock_shared();
      }
  };
  
  std::thread t1(rapid_read);
  std::thread t2(rapid_read);
  std::thread t3(rapid_read);
  
  t1.join();
  t2.join();
  t3.join();
}

// Test rapid write lock/unlock
TEST_F(RWLockTest, RapidWriteLocks) {
  rw_lock lock;
  
  auto rapid_write = [&]() {
      for (int i = 0; i < 2000; ++i) {
          lock.lock();
          lock.unlock();
      }
  };
  
  std::thread t1(rapid_write);
  std::thread t2(rapid_write);
  
  t1.join();
  t2.join();
}

// Test mixed rapid operations
TEST_F(RWLockTest, MixedRapidOperations) {
  rw_lock lock;
  
  auto rapid_read = [&]() {
      for (int i = 0; i < 1000; ++i) {
          lock.lock_shared();
          lock.unlock_shared();
      }
  };
  
  auto rapid_write = [&]() {
      for (int i = 0; i < 500; ++i) {
          lock.lock();
          lock.unlock();
      }
  };
  
  std::thread r1(rapid_read);
  std::thread r2(rapid_read);
  std::thread w1(rapid_write);
  
  r1.join();
  r2.join();
  w1.join();
}

// Test write lock doesn't allow concurrent readers
TEST_F(RWLockTest, WriteLockBlocksReaders) {
  rw_lock lock;
  std::atomic<bool> write_locked{false};
  std::atomic<bool> reader_entered{false};
  
  std::thread writer([&]() {
      lock.lock();
      write_locked.store(true);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      write_locked.store(false);
      lock.unlock();
  });
  
  std::thread reader([&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      
      lock.lock_shared();
      if (write_locked.load()) {
          reader_entered.store(true);
      }
      lock.unlock_shared();
  });
  
  writer.join();
  reader.join();
  
  // Reader should not have entered while writer held the lock
  EXPECT_FALSE(reader_entered.load());
}

// Test multiple write lock upgrades
TEST_F(RWLockTest, MultipleWriteLockPattern) {
  rw_lock lock;
  int data = 0;
  
  for (int i = 0; i < 100; ++i) {
      // Read
      lock.lock_shared();
      int temp = data;
      lock.unlock_shared();
      
      // Write
      lock.lock();
      data = temp + 1;
      lock.unlock();
  }
  
  EXPECT_EQ(data, 100);
}

// Test concurrent mixed operations
TEST_F(RWLockTest, ConcurrentMixedOperations) {
  rw_lock lock;
  std::atomic<int> data{0};
  std::atomic<int> reads{0};
  std::atomic<int> writes{0};
  
  auto mixed_task = [&](int id) {
      for (int i = 0; i < 50; ++i) {
          if (i % 3 == 0) {
              // Write operation
              lock.lock();
              data.store(data.load() + 1);
              ++writes;
              lock.unlock();
          } else {
              // Read operation
              lock.lock_shared();
              int val = data.load();
              ++reads;
              lock.unlock_shared();
          }
          std::this_thread::yield();
      }
  };
  
  std::thread t1(mixed_task, 1);
  std::thread t2(mixed_task, 2);
  std::thread t3(mixed_task, 3);
  std::thread t4(mixed_task, 4);
  
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  
  EXPECT_GT(reads.load(), 0);
  EXPECT_GT(writes.load(), 0);
}

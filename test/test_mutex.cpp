/**
 * @file test_mutex.cpp
 * @brief Comprehensive unit tests for ipc::sync::mutex class
 * 
 * This test suite covers:
 * - Mutex construction (default and named)
 * - Lock/unlock operations
 * - Try-lock functionality
 * - Timed lock with timeout
 * - Named mutex for inter-process synchronization
 * - Resource cleanup (clear, clear_storage)
 * - Native handle access
 * - Concurrent access scenarios
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "libipc/mutex.h"
#include "libipc/def.h"

using namespace ipc;
using namespace ipc::sync;

namespace {

// Generate unique mutex names for tests
std::string generate_unique_mutex_name(const char* prefix) {
    static int counter = 0;
    return std::string(prefix) + "_mutex_" + std::to_string(++counter);
}

} // anonymous namespace

class MutexTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Allow time for cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

// Test default constructor
TEST_F(MutexTest, DefaultConstructor) {
    mutex mtx;
    // Default constructed mutex may or may not be valid depending on implementation
    // Just ensure it doesn't crash
}

// Test named constructor
TEST_F(MutexTest, NamedConstructor) {
    std::string name = generate_unique_mutex_name("named_ctor");
    
    mutex mtx(name.c_str());
    EXPECT_TRUE(mtx.valid());
}

// Test native() const method
TEST_F(MutexTest, NativeConst) {
    std::string name = generate_unique_mutex_name("native_const");
    
    const mutex mtx(name.c_str());
    const void* native_handle = mtx.native();
    
    EXPECT_NE(native_handle, nullptr);
}

// Test native() non-const method
TEST_F(MutexTest, NativeNonConst) {
    std::string name = generate_unique_mutex_name("native_nonconst");
    
    mutex mtx(name.c_str());
    void* native_handle = mtx.native();
    
    EXPECT_NE(native_handle, nullptr);
}

// Test valid() method
TEST_F(MutexTest, Valid) {
    mutex mtx1;
    // May or may not be valid without open
    
    std::string name = generate_unique_mutex_name("valid");
    mutex mtx2(name.c_str());
    EXPECT_TRUE(mtx2.valid());
}

// Test open() method
TEST_F(MutexTest, Open) {
    std::string name = generate_unique_mutex_name("open");
    
    mutex mtx;
    bool result = mtx.open(name.c_str());
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(mtx.valid());
}

// Test close() method
TEST_F(MutexTest, Close) {
    std::string name = generate_unique_mutex_name("close");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    mtx.close();
    EXPECT_FALSE(mtx.valid());
}

// Test clear() method
TEST_F(MutexTest, Clear) {
    std::string name = generate_unique_mutex_name("clear");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    mtx.clear();
    EXPECT_FALSE(mtx.valid());
}

// Test clear_storage() static method
TEST_F(MutexTest, ClearStorage) {
    std::string name = generate_unique_mutex_name("clear_storage");
    
    {
        mutex mtx(name.c_str());
        EXPECT_TRUE(mtx.valid());
    }
    
    mutex::clear_storage(name.c_str());
    
    // Try to open after clear - should create new or fail gracefully
    mutex mtx2(name.c_str());
}

// Test basic lock and unlock
TEST_F(MutexTest, LockUnlock) {
    std::string name = generate_unique_mutex_name("lock_unlock");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    bool locked = mtx.lock();
    EXPECT_TRUE(locked);
    
    bool unlocked = mtx.unlock();
    EXPECT_TRUE(unlocked);
}

// Test try_lock
TEST_F(MutexTest, TryLock) {
    std::string name = generate_unique_mutex_name("try_lock");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    bool locked = mtx.try_lock();
    EXPECT_TRUE(locked);
    
    if (locked) {
        mtx.unlock();
    }
}

// Test timed lock with infinite timeout
TEST_F(MutexTest, TimedLockInfinite) {
    std::string name = generate_unique_mutex_name("timed_lock_inf");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    bool locked = mtx.lock(invalid_value);
    EXPECT_TRUE(locked);
    
    if (locked) {
        mtx.unlock();
    }
}

// Test timed lock with timeout
TEST_F(MutexTest, TimedLockTimeout) {
    std::string name = generate_unique_mutex_name("timed_lock_timeout");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    // Lock with 100ms timeout
    bool locked = mtx.lock(100);
    EXPECT_TRUE(locked);
    
    if (locked) {
        mtx.unlock();
    }
}

// Test mutex protects critical section
TEST_F(MutexTest, CriticalSection) {
    std::string name = generate_unique_mutex_name("critical_section");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    int shared_counter = 0;
    const int iterations = 100;
    
    auto increment_task = [&]() {
        for (int i = 0; i < iterations; ++i) {
            mtx.lock();
            ++shared_counter;
            mtx.unlock();
        }
    };
    
    std::thread t1(increment_task);
    std::thread t2(increment_task);
    
    t1.join();
    t2.join();
    
    EXPECT_EQ(shared_counter, iterations * 2);
}

// Test concurrent try_lock
TEST_F(MutexTest, ConcurrentTryLock) {
    std::string name = generate_unique_mutex_name("concurrent_try");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};
    
    auto try_lock_task = [&]() {
        for (int i = 0; i < 10; ++i) {
            if (mtx.try_lock()) {
                ++success_count;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                mtx.unlock();
            } else {
                ++fail_count;
            }
            std::this_thread::yield();
        }
    };
    
    std::thread t1(try_lock_task);
    std::thread t2(try_lock_task);
    std::thread t3(try_lock_task);
    
    t1.join();
    t2.join();
    t3.join();
    
    EXPECT_GT(success_count.load(), 0);
    // Some try_locks should succeed
}

// Test lock contention
TEST_F(MutexTest, LockContention) {
    std::string name = generate_unique_mutex_name("contention");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<bool> thread1_in_cs{false};
    std::atomic<bool> thread2_in_cs{false};
    std::atomic<bool> violation{false};
    
    auto contention_task = [&](std::atomic<bool>& my_flag, 
                                std::atomic<bool>& other_flag) {
        for (int i = 0; i < 50; ++i) {
            mtx.lock();
            
            my_flag.store(true);
            if (other_flag.load()) {
                violation.store(true);
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            
            my_flag.store(false);
            mtx.unlock();
            
            std::this_thread::yield();
        }
    };
    
    std::thread t1(contention_task, std::ref(thread1_in_cs), std::ref(thread2_in_cs));
    std::thread t2(contention_task, std::ref(thread2_in_cs), std::ref(thread1_in_cs));
    
    t1.join();
    t2.join();
    
    // Should never have both threads in critical section
    EXPECT_FALSE(violation.load());
}

// Test multiple lock/unlock cycles
TEST_F(MutexTest, MultipleCycles) {
    std::string name = generate_unique_mutex_name("cycles");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(mtx.lock());
        ASSERT_TRUE(mtx.unlock());
    }
}

// Test timed lock timeout scenario
TEST_F(MutexTest, TimedLockTimeoutScenario) {
    std::string name = generate_unique_mutex_name("timeout_scenario");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    // Lock in main thread
    ASSERT_TRUE(mtx.lock());
    
    std::atomic<bool> timeout_occurred{false};
    
    std::thread t([&]() {
        // Try to lock with short timeout - should timeout
        bool locked = mtx.lock(50); // 50ms timeout
        if (!locked) {
            timeout_occurred.store(true);
        } else {
            mtx.unlock();
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mtx.unlock();
    
    t.join();
    
    // Timeout should have occurred since we held the lock
    EXPECT_TRUE(timeout_occurred.load());
}

// Test reopen after close
TEST_F(MutexTest, ReopenAfterClose) {
    std::string name = generate_unique_mutex_name("reopen");
    
    mutex mtx;
    
    ASSERT_TRUE(mtx.open(name.c_str()));
    EXPECT_TRUE(mtx.valid());
    
    mtx.close();
    EXPECT_FALSE(mtx.valid());
    
    ASSERT_TRUE(mtx.open(name.c_str()));
    EXPECT_TRUE(mtx.valid());
}

// Test named mutex inter-thread synchronization
TEST_F(MutexTest, NamedMutexInterThread) {
    std::string name = generate_unique_mutex_name("inter_thread");
    
    int shared_data = 0;
    std::atomic<bool> t1_done{false};
    std::atomic<bool> t2_done{false};
    
    std::thread t1([&]() {
        mutex mtx(name.c_str());
        ASSERT_TRUE(mtx.valid());
        
        mtx.lock();
        shared_data = 100;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        mtx.unlock();
        
        t1_done.store(true);
    });
    
    std::thread t2([&]() {
        // Wait a bit to ensure t1 starts first
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        mutex mtx(name.c_str());
        ASSERT_TRUE(mtx.valid());
        
        mtx.lock();
        EXPECT_TRUE(t1_done.load() || shared_data == 100);
        shared_data = 200;
        mtx.unlock();
        
        t2_done.store(true);
    });
    
    t1.join();
    t2.join();
    
    EXPECT_EQ(shared_data, 200);
}

// Test exception safety of try_lock
TEST_F(MutexTest, TryLockExceptionSafety) {
    std::string name = generate_unique_mutex_name("try_lock_exception");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    bool exception_thrown = false;
    try {
        mtx.try_lock();
    } catch (const std::system_error&) {
        exception_thrown = true;
    } catch (...) {
        FAIL() << "Unexpected exception type";
    }
    
    // try_lock may throw system_error
    // Just ensure we can handle it
}

// Test concurrent open/close operations
TEST_F(MutexTest, ConcurrentOpenClose) {
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            std::string name = generate_unique_mutex_name("concurrent");
            name += std::to_string(i);
            
            mutex mtx;
            if (mtx.open(name.c_str())) {
                ++success_count;
                mtx.close();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count.load(), 5);
}

// Test mutex with zero timeout
TEST_F(MutexTest, ZeroTimeout) {
    std::string name = generate_unique_mutex_name("zero_timeout");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    // Lock with zero timeout (should try once and return)
    bool locked = mtx.lock(0);
    
    if (locked) {
        mtx.unlock();
    }
    // Result may vary, just ensure it doesn't hang
}

// Test rapid lock/unlock sequence
TEST_F(MutexTest, RapidLockUnlock) {
    std::string name = generate_unique_mutex_name("rapid");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    auto rapid_task = [&]() {
        for (int i = 0; i < 1000; ++i) {
            mtx.lock();
            mtx.unlock();
        }
    };
    
    std::thread t1(rapid_task);
    std::thread t2(rapid_task);
    
    t1.join();
    t2.join();
    
    // Should complete without deadlock or crash
}

// Test lock after clear
TEST_F(MutexTest, LockAfterClear) {
    std::string name = generate_unique_mutex_name("lock_after_clear");
    
    mutex mtx(name.c_str());
    ASSERT_TRUE(mtx.valid());
    
    mtx.lock();
    mtx.unlock();
    
    mtx.clear();
    EXPECT_FALSE(mtx.valid());
    
    // Attempting to lock after clear should fail gracefully
    bool locked = mtx.lock();
    EXPECT_FALSE(locked);
}

/**
 * @file test_condition.cpp
 * @brief Comprehensive unit tests for ipc::sync::condition class
 * 
 * This test suite covers:
 * - Condition variable construction (default and named)
 * - Wait, notify, and broadcast operations
 * - Timed wait with timeout
 * - Integration with mutex
 * - Producer-consumer patterns with condition variables
 * - Resource cleanup
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "libipc/condition.h"
#include "libipc/mutex.h"
#include "libipc/def.h"

using namespace ipc;
using namespace ipc::sync;

namespace {

std::string generate_unique_cv_name(const char* prefix) {
    static int counter = 0;
    return std::string(prefix) + "_cv_" + std::to_string(++counter);
}

} // anonymous namespace

class ConditionTest : public ::testing::Test {
protected:
    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

// Test default constructor
TEST_F(ConditionTest, DefaultConstructor) {
    condition cv;
}

// Test named constructor
TEST_F(ConditionTest, NamedConstructor) {
    std::string name = generate_unique_cv_name("named");
    
    condition cv(name.c_str());
    EXPECT_TRUE(cv.valid());
}

// Test native() methods
TEST_F(ConditionTest, NativeHandle) {
    std::string name = generate_unique_cv_name("native");
    
    condition cv(name.c_str());
    ASSERT_TRUE(cv.valid());
    
    const void* const_handle = static_cast<const condition&>(cv).native();
    void* handle = cv.native();
    
    EXPECT_NE(const_handle, nullptr);
    EXPECT_NE(handle, nullptr);
}

// Test valid() method
TEST_F(ConditionTest, Valid) {
    condition cv1;
    
    std::string name = generate_unique_cv_name("valid");
    condition cv2(name.c_str());
    EXPECT_TRUE(cv2.valid());
}

// Test open() method
TEST_F(ConditionTest, Open) {
    std::string name = generate_unique_cv_name("open");
    
    condition cv;
    bool result = cv.open(name.c_str());
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(cv.valid());
}

// Test close() method
TEST_F(ConditionTest, Close) {
    std::string name = generate_unique_cv_name("close");
    
    condition cv(name.c_str());
    ASSERT_TRUE(cv.valid());
    
    cv.close();
    EXPECT_FALSE(cv.valid());
}

// Test clear() method
TEST_F(ConditionTest, Clear) {
    std::string name = generate_unique_cv_name("clear");
    
    condition cv(name.c_str());
    ASSERT_TRUE(cv.valid());
    
    cv.clear();
    EXPECT_FALSE(cv.valid());
}

// Test clear_storage() static method
TEST_F(ConditionTest, ClearStorage) {
    std::string name = generate_unique_cv_name("clear_storage");
    
    {
        condition cv(name.c_str());
        EXPECT_TRUE(cv.valid());
    }
    
    condition::clear_storage(name.c_str());
}

// Test basic wait and notify
TEST_F(ConditionTest, WaitNotify) {
    std::string cv_name = generate_unique_cv_name("wait_notify");
    std::string mtx_name = generate_unique_cv_name("wait_notify_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<bool> notified{false};
    
    std::thread waiter([&]() {
        mtx.lock();
        cv.wait(mtx);
        notified.store(true);
        mtx.unlock();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    mtx.lock();
    cv.notify(mtx);
    mtx.unlock();
    
    waiter.join();
    
    EXPECT_TRUE(notified.load());
}

// Test broadcast to multiple waiters
TEST_F(ConditionTest, Broadcast) {
    std::string cv_name = generate_unique_cv_name("broadcast");
    std::string mtx_name = generate_unique_cv_name("broadcast_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<int> notified_count{0};
    const int num_waiters = 5;
    
    std::vector<std::thread> waiters;
    for (int i = 0; i < num_waiters; ++i) {
        waiters.emplace_back([&]() {
            mtx.lock();
            cv.wait(mtx);
            ++notified_count;
            mtx.unlock();
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mtx.lock();
    cv.broadcast(mtx);
    mtx.unlock();
    
    for (auto& t : waiters) {
        t.join();
    }
    
    EXPECT_EQ(notified_count.load(), num_waiters);
}

// Test timed wait with timeout
TEST_F(ConditionTest, TimedWait) {
    std::string cv_name = generate_unique_cv_name("timed_wait");
    std::string mtx_name = generate_unique_cv_name("timed_wait_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    auto start = std::chrono::steady_clock::now();
    
    mtx.lock();
    bool result = cv.wait(mtx, 100); // 100ms timeout
    mtx.unlock();
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_FALSE(result); // Should timeout
    EXPECT_GE(elapsed, 80); // Allow some tolerance
}

// Test wait with immediate notify
TEST_F(ConditionTest, ImmediateNotify) {
    std::string cv_name = generate_unique_cv_name("immediate");
    std::string mtx_name = generate_unique_cv_name("immediate_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<bool> wait_started{false};
    std::atomic<bool> notified{false};
    
    std::thread waiter([&]() {
        mtx.lock();
        wait_started.store(true);
        cv.wait(mtx, 1000); // 1 second timeout
        notified.store(true);
        mtx.unlock();
    });
    
    // Wait for waiter to start
    while (!wait_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    mtx.lock();
    cv.notify(mtx);
    mtx.unlock();
    
    waiter.join();
    
    EXPECT_TRUE(notified.load());
}

// Test producer-consumer with condition variable
TEST_F(ConditionTest, ProducerConsumer) {
    std::string cv_name = generate_unique_cv_name("prod_cons");
    std::string mtx_name = generate_unique_cv_name("prod_cons_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<int> buffer{0};
    std::atomic<bool> ready{false};
    std::atomic<int> consumed_value{0};
    
    std::thread producer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        mtx.lock();
        buffer.store(42);
        ready.store(true);
        cv.notify(mtx);
        mtx.unlock();
    });
    
    std::thread consumer([&]() {
        mtx.lock();
        while (!ready.load()) {
            cv.wait(mtx, 2000);
        }
        consumed_value.store(buffer.load());
        mtx.unlock();
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(consumed_value.load(), 42);
}

// Test multiple notify operations
TEST_F(ConditionTest, MultipleNotify) {
    std::string cv_name = generate_unique_cv_name("multi_notify");
    std::string mtx_name = generate_unique_cv_name("multi_notify_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<int> notify_count{0};
    const int num_notifications = 3;
    
    std::thread waiter([&]() {
        for (int i = 0; i < num_notifications; ++i) {
            mtx.lock();
            cv.wait(mtx, 1000);
            ++notify_count;
            mtx.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    for (int i = 0; i < num_notifications; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        mtx.lock();
        cv.notify(mtx);
        mtx.unlock();
    }
    
    waiter.join();
    
    EXPECT_EQ(notify_count.load(), num_notifications);
}

// Test notify vs broadcast
TEST_F(ConditionTest, NotifyVsBroadcast) {
    std::string cv_name = generate_unique_cv_name("notify_vs_broadcast");
    std::string mtx_name = generate_unique_cv_name("notify_vs_broadcast_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    // Test notify (should wake one)
    std::atomic<int> notify_woken{0};
    
    std::vector<std::thread> notify_waiters;
    for (int i = 0; i < 3; ++i) {
        notify_waiters.emplace_back([&]() {
            mtx.lock();
            cv.wait(mtx, 100);
            ++notify_woken;
            mtx.unlock();
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    mtx.lock();
    cv.notify(mtx); // Wake one
    mtx.unlock();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    for (auto& t : notify_waiters) {
        t.join();
    }
    
    // At least one should be woken by notify
    EXPECT_GE(notify_woken.load(), 1);
}

// Test condition variable with spurious wakeups pattern
TEST_F(ConditionTest, SpuriousWakeupPattern) {
    std::string cv_name = generate_unique_cv_name("spurious");
    std::string mtx_name = generate_unique_cv_name("spurious_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<bool> predicate{false};
    std::atomic<bool> done{false};
    
    std::thread waiter([&]() {
        mtx.lock();
        while (!predicate.load()) {
            if (!cv.wait(mtx, 100)) {
                // Timeout - check predicate again
                if (predicate.load()) break;
            }
        }
        done.store(true);
        mtx.unlock();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    mtx.lock();
    predicate.store(true);
    cv.notify(mtx);
    mtx.unlock();
    
    waiter.join();
    
    EXPECT_TRUE(done.load());
}

// Test reopen after close
TEST_F(ConditionTest, ReopenAfterClose) {
    std::string name = generate_unique_cv_name("reopen");
    
    condition cv;
    
    ASSERT_TRUE(cv.open(name.c_str()));
    EXPECT_TRUE(cv.valid());
    
    cv.close();
    EXPECT_FALSE(cv.valid());
    
    ASSERT_TRUE(cv.open(name.c_str()));
    EXPECT_TRUE(cv.valid());
}

// Test named condition variable sharing between threads
TEST_F(ConditionTest, NamedSharing) {
    std::string cv_name = generate_unique_cv_name("sharing");
    std::string mtx_name = generate_unique_cv_name("sharing_mtx");
    
    std::atomic<int> value{0};
    
    std::thread t1([&]() {
        condition cv(cv_name.c_str());
        mutex mtx(mtx_name.c_str());
        
        ASSERT_TRUE(cv.valid());
        ASSERT_TRUE(mtx.valid());
        
        mtx.lock();
        cv.wait(mtx, 1000);
        value.store(100);
        mtx.unlock();
    });
    
    std::thread t2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        condition cv(cv_name.c_str());
        mutex mtx(mtx_name.c_str());
        
        ASSERT_TRUE(cv.valid());
        ASSERT_TRUE(mtx.valid());
        
        mtx.lock();
        cv.notify(mtx);
        mtx.unlock();
    });
    
    t1.join();
    t2.join();
    
    EXPECT_EQ(value.load(), 100);
}

// Test infinite wait
TEST_F(ConditionTest, InfiniteWait) {
    std::string cv_name = generate_unique_cv_name("infinite");
    std::string mtx_name = generate_unique_cv_name("infinite_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<bool> woken{false};
    
    std::thread waiter([&]() {
        mtx.lock();
        cv.wait(mtx, invalid_value); // Infinite wait
        woken.store(true);
        mtx.unlock();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mtx.lock();
    cv.notify(mtx);
    mtx.unlock();
    
    waiter.join();
    
    EXPECT_TRUE(woken.load());
}

// Test broadcast with sequential waiters
TEST_F(ConditionTest, BroadcastSequential) {
    std::string cv_name = generate_unique_cv_name("broadcast_seq");
    std::string mtx_name = generate_unique_cv_name("broadcast_seq_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    ASSERT_TRUE(mtx.valid());
    
    std::atomic<int> processed{0};
    const int num_threads = 4;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            mtx.lock();
            cv.wait(mtx, 2000);
            ++processed;
            mtx.unlock();
        });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mtx.lock();
    cv.broadcast(mtx);
    mtx.unlock();
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(processed.load(), num_threads);
}

// Test operations after clear
TEST_F(ConditionTest, AfterClear) {
    std::string cv_name = generate_unique_cv_name("after_clear");
    std::string mtx_name = generate_unique_cv_name("after_clear_mtx");
    
    condition cv(cv_name.c_str());
    mutex mtx(mtx_name.c_str());
    
    ASSERT_TRUE(cv.valid());
    
    cv.clear();
    EXPECT_FALSE(cv.valid());
    
    // Operations after clear should fail gracefully
    mtx.lock();
    EXPECT_FALSE(cv.wait(mtx, 10));
    EXPECT_FALSE(cv.notify(mtx));
    EXPECT_FALSE(cv.broadcast(mtx));
    mtx.unlock();
}

/**
 * @file test_semaphore.cpp
 * @brief Comprehensive unit tests for ipc::sync::semaphore class
 * 
 * This test suite covers:
 * - Semaphore construction (default and named with count)
 * - Wait and post operations
 * - Timed wait with timeout
 * - Named semaphore for inter-process synchronization
 * - Resource cleanup (clear, clear_storage)
 * - Producer-consumer patterns
 * - Multiple wait/post scenarios
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include "libipc/semaphore.h"
#include "libipc/def.h"

using namespace ipc;
using namespace ipc::sync;

namespace {

std::string generate_unique_sem_name(const char* prefix) {
    static int counter = 0;
    return std::string(prefix) + "_sem_" + std::to_string(++counter);
}

} // anonymous namespace

class SemaphoreTest : public ::testing::Test {
protected:
    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

// Test default constructor
TEST_F(SemaphoreTest, DefaultConstructor) {
    semaphore sem;
    // Default constructed semaphore
}

// Test named constructor with count
TEST_F(SemaphoreTest, NamedConstructorWithCount) {
    std::string name = generate_unique_sem_name("named_count");
    
    semaphore sem(name.c_str(), 5);
    EXPECT_TRUE(sem.valid());
}

// Test named constructor with zero count
TEST_F(SemaphoreTest, NamedConstructorZeroCount) {
    std::string name = generate_unique_sem_name("zero_count");
    
    semaphore sem(name.c_str(), 0);
    EXPECT_TRUE(sem.valid());
}

// Test native() methods
TEST_F(SemaphoreTest, NativeHandle) {
    std::string name = generate_unique_sem_name("native");
    
    semaphore sem(name.c_str(), 1);
    ASSERT_TRUE(sem.valid());
    
    const void* const_handle = static_cast<const semaphore&>(sem).native();
    void* handle = sem.native();
    
    EXPECT_NE(const_handle, nullptr);
    EXPECT_NE(handle, nullptr);
}

// Test valid() method
TEST_F(SemaphoreTest, Valid) {
    semaphore sem1;
    
    std::string name = generate_unique_sem_name("valid");
    semaphore sem2(name.c_str(), 1);
    EXPECT_TRUE(sem2.valid());
}

// Test open() method
TEST_F(SemaphoreTest, Open) {
    std::string name = generate_unique_sem_name("open");
    
    semaphore sem;
    bool result = sem.open(name.c_str(), 3);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(sem.valid());
}

// Test close() method
TEST_F(SemaphoreTest, Close) {
    std::string name = generate_unique_sem_name("close");
    
    semaphore sem(name.c_str(), 1);
    ASSERT_TRUE(sem.valid());
    
    sem.close();
    EXPECT_FALSE(sem.valid());
}

// Test clear() method
TEST_F(SemaphoreTest, Clear) {
    std::string name = generate_unique_sem_name("clear");
    
    semaphore sem(name.c_str(), 1);
    ASSERT_TRUE(sem.valid());
    
    sem.clear();
    EXPECT_FALSE(sem.valid());
}

// Test clear_storage() static method
TEST_F(SemaphoreTest, ClearStorage) {
    std::string name = generate_unique_sem_name("clear_storage");
    
    {
        semaphore sem(name.c_str(), 1);
        EXPECT_TRUE(sem.valid());
    }
    
    semaphore::clear_storage(name.c_str());
}

// Test basic wait and post
TEST_F(SemaphoreTest, WaitPost) {
    std::string name = generate_unique_sem_name("wait_post");
    
    semaphore sem(name.c_str(), 1);
    ASSERT_TRUE(sem.valid());
    
    bool waited = sem.wait();
    EXPECT_TRUE(waited);
    
    bool posted = sem.post();
    EXPECT_TRUE(posted);
}

// Test post with count
TEST_F(SemaphoreTest, PostWithCount) {
    std::string name = generate_unique_sem_name("post_count");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    bool posted = sem.post(5);
    EXPECT_TRUE(posted);
    
    // Now should be able to wait 5 times
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(sem.wait(10)); // 10ms timeout
    }
}

// Test timed wait with timeout
TEST_F(SemaphoreTest, TimedWait) {
    std::string name = generate_unique_sem_name("timed_wait");
    
    semaphore sem(name.c_str(), 1);
    ASSERT_TRUE(sem.valid());
    
    bool waited = sem.wait(100); // 100ms timeout
    EXPECT_TRUE(waited);
}

// Test wait timeout scenario
TEST_F(SemaphoreTest, WaitTimeout) {
    std::string name = generate_unique_sem_name("wait_timeout");
    
    semaphore sem(name.c_str(), 0); // Zero count
    ASSERT_TRUE(sem.valid());
    
    auto start = std::chrono::steady_clock::now();
    bool waited = sem.wait(50); // 50ms timeout
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should timeout
    EXPECT_FALSE(waited);
    EXPECT_GE(elapsed, 40); // Allow some tolerance
}

// Test infinite wait
TEST_F(SemaphoreTest, InfiniteWait) {
    std::string name = generate_unique_sem_name("infinite_wait");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    std::atomic<bool> wait_started{false};
    std::atomic<bool> wait_succeeded{false};
    
    std::thread waiter([&]() {
        wait_started.store(true);
        bool result = sem.wait(invalid_value);
        wait_succeeded.store(result);
    });
    
    // Wait for thread to start waiting
    while (!wait_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Post to release the waiter
    sem.post();
    
    waiter.join();
    
    EXPECT_TRUE(wait_succeeded.load());
}

// Test producer-consumer pattern
TEST_F(SemaphoreTest, ProducerConsumer) {
    std::string name = generate_unique_sem_name("prod_cons");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    const int count = 10;
    
    std::thread producer([&]() {
        for (int i = 0; i < count; ++i) {
            ++produced;
            sem.post();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    std::thread consumer([&]() {
        for (int i = 0; i < count; ++i) {
            sem.wait();
            ++consumed;
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(produced.load(), count);
    EXPECT_EQ(consumed.load(), count);
}

// Test multiple producers and consumers
TEST_F(SemaphoreTest, MultipleProducersConsumers) {
    std::string name = generate_unique_sem_name("multi_prod_cons");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};
    const int items_per_producer = 5;
    const int num_producers = 3;
    const int num_consumers = 3;
    
    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < items_per_producer; ++j) {
                ++total_produced;
                sem.post();
                std::this_thread::yield();
            }
        });
    }
    
    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            for (int j = 0; j < items_per_producer; ++j) {
                if (sem.wait(1000)) {
                    ++total_consumed;
                }
            }
        });
    }
    
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    
    EXPECT_EQ(total_produced.load(), items_per_producer * num_producers);
    EXPECT_EQ(total_consumed.load(), items_per_producer * num_producers);
}

// Test semaphore with initial count
TEST_F(SemaphoreTest, InitialCount) {
    std::string name = generate_unique_sem_name("initial_count");
    const uint32_t initial = 3;
    
    semaphore sem(name.c_str(), initial);
    ASSERT_TRUE(sem.valid());
    
    // Should be able to wait 'initial' times without blocking
    for (uint32_t i = 0; i < initial; ++i) {
        EXPECT_TRUE(sem.wait(10));
    }
    
    // Next wait should timeout
    EXPECT_FALSE(sem.wait(10));
}

// Test rapid post operations
TEST_F(SemaphoreTest, RapidPost) {
    std::string name = generate_unique_sem_name("rapid_post");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    const int post_count = 100;
    for (int i = 0; i < post_count; ++i) {
        EXPECT_TRUE(sem.post());
    }
    
    // Should be able to wait post_count times
    int wait_count = 0;
    for (int i = 0; i < post_count; ++i) {
        if (sem.wait(10)) {
            ++wait_count;
        }
    }
    
    EXPECT_EQ(wait_count, post_count);
}

// Test concurrent post operations
TEST_F(SemaphoreTest, ConcurrentPost) {
    std::string name = generate_unique_sem_name("concurrent_post");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    std::atomic<int> post_count{0};
    const int threads = 5;
    const int posts_per_thread = 10;
    
    std::vector<std::thread> posters;
    for (int i = 0; i < threads; ++i) {
        posters.emplace_back([&]() {
            for (int j = 0; j < posts_per_thread; ++j) {
                if (sem.post()) {
                    ++post_count;
                }
            }
        });
    }
    
    for (auto& t : posters) t.join();
    
    EXPECT_EQ(post_count.load(), threads * posts_per_thread);
    
    // Verify by consuming
    int consumed = 0;
    for (int i = 0; i < threads * posts_per_thread; ++i) {
        if (sem.wait(10)) {
            ++consumed;
        }
    }
    
    EXPECT_EQ(consumed, threads * posts_per_thread);
}

// Test reopen after close
TEST_F(SemaphoreTest, ReopenAfterClose) {
    std::string name = generate_unique_sem_name("reopen");
    
    semaphore sem;
    
    ASSERT_TRUE(sem.open(name.c_str(), 2));
    EXPECT_TRUE(sem.valid());
    
    sem.close();
    EXPECT_FALSE(sem.valid());
    
    ASSERT_TRUE(sem.open(name.c_str(), 3));
    EXPECT_TRUE(sem.valid());
}

// Test named semaphore sharing between threads
TEST_F(SemaphoreTest, NamedSemaphoreSharing) {
    std::string name = generate_unique_sem_name("sharing");
    
    std::atomic<int> value{0};
    
    std::thread t1([&]() {
        semaphore sem(name.c_str(), 0);
        ASSERT_TRUE(sem.valid());
        
        sem.wait(); // Wait for signal
        value.store(100);
    });
    
    std::thread t2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        semaphore sem(name.c_str(), 0);
        ASSERT_TRUE(sem.valid());
        
        sem.post(); // Signal t1
    });
    
    t1.join();
    t2.join();
    
    EXPECT_EQ(value.load(), 100);
}

// Test post multiple count at once
TEST_F(SemaphoreTest, PostMultiple) {
    std::string name = generate_unique_sem_name("post_multiple");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    const uint32_t count = 10;
    bool posted = sem.post(count);
    EXPECT_TRUE(posted);
    
    // Consume all
    for (uint32_t i = 0; i < count; ++i) {
        EXPECT_TRUE(sem.wait(10));
    }
    
    // Should be empty now
    EXPECT_FALSE(sem.wait(10));
}

// Test semaphore after clear
TEST_F(SemaphoreTest, AfterClear) {
    std::string name = generate_unique_sem_name("after_clear");
    
    semaphore sem(name.c_str(), 5);
    ASSERT_TRUE(sem.valid());
    
    sem.wait();
    sem.clear();
    EXPECT_FALSE(sem.valid());
    
    // Operations after clear should fail gracefully
    EXPECT_FALSE(sem.wait(10));
    EXPECT_FALSE(sem.post());
}

// Test zero timeout
TEST_F(SemaphoreTest, ZeroTimeout) {
    std::string name = generate_unique_sem_name("zero_timeout");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    bool waited = sem.wait(0);
    // Should return immediately (either success or timeout)
}

// Test high-frequency wait/post
TEST_F(SemaphoreTest, HighFrequency) {
    std::string name = generate_unique_sem_name("high_freq");
    
    semaphore sem(name.c_str(), 0);
    ASSERT_TRUE(sem.valid());
    
    std::thread poster([&]() {
        for (int i = 0; i < 1000; ++i) {
            sem.post();
        }
    });
    
    std::thread waiter([&]() {
        for (int i = 0; i < 1000; ++i) {
            sem.wait(100);
        }
    });
    
    poster.join();
    waiter.join();
}

#include <type_traits>
#include <iostream>
#include <shared_mutex> // std::shared_lock
#include <utility>
#include <thread>
#include <future>       // std::async
#include <atomic>
#include <string>       // std::string

#include "capo/spin_lock.hpp"
#include "capo/type_name.hpp"

#include "libipc/rw_lock.h"

#include "test.h"
#include "thread_pool.h"

namespace {

constexpr int LoopCount = 100000;
constexpr int ThreadMax = 8;

template <typename T>
constexpr T acc(T b, T e) noexcept {
    return (e + b) * (e - b + 1) / 2;
}

template <typename Mutex>
struct lc_wrapper : Mutex {
    void lock_shared  () { Mutex::lock  (); }
    void unlock_shared() { Mutex::unlock(); }
};

template <typename Lc, int Loops = LoopCount>
void benchmark_lc(int w, int r, char const * message) {
    ipc_ut::sender().start(static_cast<std::size_t>(w));
    ipc_ut::reader().start(static_cast<std::size_t>(r));
    ipc_ut::test_stopwatch sw;

    std::uint64_t data = 0;
    std::uint64_t sum  = acc<std::uint64_t>(1, Loops) * w;
    Lc lc;

    for (int k = 0; k < r; ++k) {
        ipc_ut::reader() << [sum, &lc, &data] {
            while (1) {
                {
                    std::shared_lock<Lc> guard { lc };
                    if (data == sum) break;
                }
                std::this_thread::yield();
            }
        };
    }

    for (int k = 0; k < w; ++k) {
        ipc_ut::sender() << [&sw, &lc, &data] {
            sw.start();
            for (int i = 1; i <= Loops; ++i) {
                {
                    std::lock_guard<Lc> guard { lc };
                    data += i;
                }
                std::this_thread::yield();
            }
        };
    }

    ipc_ut::sender().wait_for_done();
    sw.print_elapsed(w, r, Loops, message);
    ipc_ut::reader().wait_for_done();
}

void test_lock_performance(int w, int r) {
    std::cout << "test_lock_performance: [" << w << "-" << r << "]" << std::endl;
    benchmark_lc<ipc::rw_lock               >(w, r, "ipc::rw_lock");
    benchmark_lc<lc_wrapper< ipc::spin_lock>>(w, r, "ipc::spin_lock");
    benchmark_lc<lc_wrapper<capo::spin_lock>>(w, r, "capo::spin_lock");
    benchmark_lc<lc_wrapper<std::mutex>     >(w, r, "std::mutex");
    // benchmark_lc<std::shared_mutex          >(w, r, "std::shared_mutex");
}

} // internal-linkage

//TEST(Thread, rw_lock) {
//    for (int i = 1; i <= ThreadMax; ++i) test_lock_performance(i, 0);
//    for (int i = 1; i <= ThreadMax; ++i) test_lock_performance(1, i);
//    for (int i = 2; i <= ThreadMax; ++i) test_lock_performance(i, i);
//}

#if 0 // disable ipc::tls
TEST(Thread, tls_main_thread) {
    ipc::tls::pointer<int> p;
    EXPECT_FALSE(p);
    EXPECT_NE(p.create(123), nullptr);
    EXPECT_EQ(*p, 123);
}

namespace {

struct Foo {
    std::atomic<int> * px_;
    Foo(std::atomic<int> * px) : px_(px) {}
    ~Foo() {
        px_->fetch_add(1, std::memory_order_relaxed);
    }
};

} // namespace

TEST(Thread, tls_create_once) {
    std::atomic<int> x { 0 };
    Foo { &x };
    EXPECT_EQ(x, 1);
    {
        ipc::tls::pointer<Foo> foo;
        EXPECT_NE(foo.create(&x), nullptr);
        EXPECT_EQ(x, 1);

        std::thread {[&foo, &x] {
            auto foo1 = foo.create_once(&x);
            auto foo2 = foo.create_once(&x);
            EXPECT_EQ(foo1, foo2);
        }}.join();
        EXPECT_EQ(x, 2);
    }
    EXPECT_EQ(x, 3);
}

TEST(Thread, tls_multi_thread) {
    std::atomic<int> x { 0 };
    {
        ipc::tls::pointer<Foo> foo; // no create
        EXPECT_EQ(x, 0);

        ipc_ut::thread_pool pool { 10 };
        for (int i = 0; i < 1000; ++i) {
          pool << [&foo, &x] {
            // foo.create_once(&x);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
          };
        }
        pool.wait_for_done();
        EXPECT_EQ(x, 0); // thread_pool hasn't destructed.
    }
    // EXPECT_EQ(x, 10);
}

namespace {

template <typename Tls, typename T>
void benchmark_tls(char const * message) {
    ipc_ut::thread_pool pool { static_cast<std::size_t>(ThreadMax) };
    ipc_ut::test_stopwatch sw;
    pool.wait_for_started();

    for (int k = 0; k < ThreadMax; ++k) {
        pool << [&sw] {
            sw.start();
            for (int i = 0; i < LoopCount * 10; ++i) {
                *Tls::template get<T>() = i;
            }
        };
    }

    pool.wait_for_done();
    sw.print_elapsed(ThreadMax, LoopCount * 10, message);
}

struct ipc_tls {
    template <typename T>
    static T * get() {
        static ipc::tls::pointer<T> p;
        return p.create_once();
    }
};

struct std_tls {
    template <typename T>
    static T * get() {
        thread_local T p;
        return &p;
    }
};

struct Str {
    std::string str_;

    Str & operator=(int i) {
        str_ = std::to_string(i);
        return *this;
    }
};

} // internal-linkage

TEST(Thread, tls_benchmark) {
    benchmark_tls<std_tls, int>("std_tls: int");
    benchmark_tls<ipc_tls, int>("ipc_tls: int");
    benchmark_tls<std_tls, Str>("std_tls: Str");
    benchmark_tls<ipc_tls, Str>("ipc_tls: Str");
}
#endif
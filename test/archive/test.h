#pragma once

#include <iostream>
#include <atomic>
#include <thread>
#include <string>
#include <memory>
#include <mutex>
#include <utility>

#include "gtest/gtest.h"

#include "capo/stopwatch.hpp"

#include "thread_pool.h"

#include "libipc/platform/detail.h"
#ifdef LIBIPC_OS_LINUX
#include <fcntl.h> // ::open
#endif

namespace ipc_ut {

template <typename Dur>
struct unit;

template <> struct unit<std::chrono::nanoseconds> {
    constexpr static char const * str() noexcept {
        return "ns";
    }
};

template <> struct unit<std::chrono::microseconds> {
    constexpr static char const * str() noexcept {
        return "us";
    }
};

template <> struct unit<std::chrono::milliseconds> {
    constexpr static char const * str() noexcept {
        return "ms";
    }
};

template <> struct unit<std::chrono::seconds> {
    constexpr static char const * str() noexcept {
        return "sec";
    }
};

struct test_stopwatch {
    capo::stopwatch<> sw_;
    std::atomic_flag started_ = ATOMIC_FLAG_INIT;

    void start() {
        if (!started_.test_and_set()) {
            sw_.start();
        }
    }

    template <typename ToDur = std::chrono::nanoseconds>
    void print_elapsed(int N, int Loops, char const * message = "") {
        auto ts = sw_.elapsed<ToDur>();
        std::cout << "[" << N << ", \t" << Loops << "] " << message << "\t"
                  << (double(ts) / double(Loops)) << " " << unit<ToDur>::str() << std::endl;
    }

    template <int Factor, typename ToDur = std::chrono::nanoseconds>
    void print_elapsed(int N, int M, int Loops, char const * message = "") {
        auto ts = sw_.elapsed<ToDur>();
        std::cout << "[" << N << "-" << M << ", \t" << Loops << "] " << message << "\t"
                  << (double(ts) / double(Factor ? (Loops * Factor) : (Loops * N))) << " " << unit<ToDur>::str() << std::endl;
    }

    template <typename ToDur = std::chrono::nanoseconds>
    void print_elapsed(int N, int M, int Loops, char const * message = "") {
        print_elapsed<0, ToDur>(N, M, Loops, message);
    }
};

inline static thread_pool & sender() {
    static thread_pool pool;
    return pool;
}

inline static thread_pool & reader() {
    static thread_pool pool;
    return pool;
}

#ifdef LIBIPC_OS_LINUX
inline bool check_exist(char const *name) noexcept {
    int fd = ::open((std::string{"/dev/shm/"} + name).c_str(), O_RDONLY);
    if (fd == -1) {
        return false;
    }
    ::close(fd);
    return true;
}
#endif

inline bool expect_exist(char const *name, bool expected) noexcept {
#ifdef LIBIPC_OS_LINUX
    return ipc_ut::check_exist(name) == expected;
#else
    return true;
#endif
}

} // namespace ipc_ut

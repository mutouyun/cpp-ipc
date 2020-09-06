#include <vector>
#include <array>
#include <thread>
#include <atomic>
#include <cstddef>

#include "capo/random.hpp"

#include "memory/resource.h"
#include "pool_alloc.h"

// #include "gperftools/tcmalloc.h"

#include "test.h"

namespace {

constexpr int DataMin   = 4;
constexpr int DataMax   = 256;
constexpr int LoopCount = 4194304;

// constexpr int DataMin   = 256;
// constexpr int DataMax   = 512;
// constexpr int LoopCount = 2097152;

std::vector<std::size_t> sizes__;

template <typename M>
struct alloc_ix_t {
    static std::vector<int> ix_;
    static bool inited_;

    alloc_ix_t() {
        if (inited_) return;
        inited_ = true;
        M::init(ix_);
    }

    int index(std::size_t /*pid*/, std::size_t /*k*/, std::size_t n) {
        return ix_[n];
    }
};

template <typename M>
std::vector<int> alloc_ix_t<M>::ix_(LoopCount);
template <typename M>
bool alloc_ix_t<M>::inited_ = false;

template <std::size_t N>
struct alloc_FIFO : alloc_ix_t<alloc_FIFO<N>> {
    static void init(std::vector<int>& ix) {
        for (int i = 0; i < LoopCount; ++i) {
            ix[static_cast<std::size_t>(i)] = i;
        }
    }
};

template <std::size_t N>
struct alloc_LIFO : alloc_ix_t<alloc_LIFO<N>> {
    static void init(std::vector<int>& ix) {
        for (int i = 0; i < LoopCount; ++i) {
            ix[static_cast<std::size_t>(i)] = i;
        }
    }

    int index(std::size_t pid, std::size_t k, std::size_t n) {
        constexpr static int CacheSize = LoopCount / N;
        if (k) {
            return this->ix_[(CacheSize * (2 * pid + 1)) - 1 - n];
        }
        else return this->ix_[n];
    }
};

template <std::size_t N>
struct alloc_random : alloc_ix_t<alloc_random<N>> {
    static void init(std::vector<int>& ix) {
        capo::random<> rdm_index(0, LoopCount - 1);
        for (int i = 0; i < LoopCount; ++i) {
            ix[static_cast<std::size_t>(i)] = rdm_index();
        }
    }
};

struct Init {
    Init() {
        capo::random<> rdm{ DataMin, DataMax };
        for (int i = 0; i < LoopCount; ++i) {
            sizes__.emplace_back(static_cast<std::size_t>(rdm()));
        }
    }
} init__;

template <typename AllocT, int ThreadsN>
void benchmark_alloc() {
    std::cout << std::endl
              << "[Threads: " << ThreadsN << "] "
              << type_name<AllocT>() << std::endl;

    constexpr static int CacheSize = LoopCount / ThreadsN;

    std::atomic_int fini { 0 };
    test_stopwatch sw;

    std::thread works[ThreadsN];
    int pid = 0;

    for (auto& w : works) {
        w = std::thread {[&, pid] {
            sw.start();
            for (std::size_t k = 0; k < 100; ++k)
            for (int n = (CacheSize * pid); n < (CacheSize * (pid + 1)); ++n) {
                std::size_t s = sizes__[n];
                AllocT::free(AllocT::alloc(s), s);
            }
            if ((fini.fetch_add(1, std::memory_order_relaxed) + 1) == ThreadsN) {
                sw.print_elapsed<1, std::chrono::nanoseconds>(DataMin, DataMax, LoopCount * 100, " ns/d");
            }
        }};
        ++pid;
    }

    for (auto& w : works) w.join();
}

template <typename AllocT, template <std::size_t> class ModeT, int ThreadsN>
void benchmark_alloc() {
    std::cout << std::endl
              << "[Threads: " << ThreadsN << ", Mode: " << type_name<ModeT<ThreadsN>>() << "] "
              << type_name<AllocT>() << std::endl;

    constexpr static int CacheSize = LoopCount / ThreadsN;

    std::vector<void*> ptrs[ThreadsN];
    for (auto& vec : ptrs) {
        vec.resize(LoopCount);
    }

    ModeT<ThreadsN> mode;

    std::atomic_int fini { 0 };
    test_stopwatch  sw;

    std::thread works[ThreadsN];
    int pid = 0;

    for (auto& w : works) {
        w = std::thread {[&, pid] {
            auto& vec = ptrs[pid];
            sw.start();
            for (std::size_t k = 0; k < 2; ++k)
            for (int n = (CacheSize * pid); n < (CacheSize * (pid + 1)); ++n) {
                int    m = mode.index(pid, k, n);
                void*& p = vec[static_cast<std::size_t>(m)];
                std::size_t s  = sizes__[static_cast<std::size_t>(m)];
                if (p == nullptr) {
                    p = AllocT::alloc(s);
                }
                else {
                    AllocT::free(p, s);
                    p = nullptr;
                }
            }
            if ((fini.fetch_add(1, std::memory_order_relaxed) + 1) == ThreadsN) {
                sw.print_elapsed<1>(DataMin, DataMax, LoopCount);
            }
        }};
        ++pid;
    }

    for (auto& w : works) w.join();
}

template <typename AllocT, template <std::size_t> class ModeT, int ThreadsN>
struct test_performance {
    static void start() {
        test_performance<AllocT, ModeT, ThreadsN / 2>::start();
        benchmark_alloc<AllocT, ModeT, ThreadsN>();
    }
};

template <typename AllocT, template <std::size_t> class ModeT>
struct test_performance<AllocT, ModeT, 1> {
    static void start() {
        benchmark_alloc<AllocT, ModeT, 1>();
    }
};

template <std::size_t> struct dummy;

template <typename AllocT, int ThreadsN>
struct test_performance<AllocT, dummy, ThreadsN> {
    static void start() {
        test_performance<AllocT, dummy, ThreadsN / 2>::start();
        benchmark_alloc<AllocT, ThreadsN>();
    }
};

template <typename AllocT>
struct test_performance<AllocT, dummy, 1> {
    static void start() {
        benchmark_alloc<AllocT, 1>();
    }
};

// class tc_alloc {
// public:
//    static void clear() {}

//    static void* alloc(std::size_t size) {
//        return size ? tc_malloc(size) : nullptr;
//    }

//    static void free(void* p, std::size_t size) {
//        tc_free_sized(p, size);
//    }
// };

TEST(Memory, static_alloc) {
    // test_performance<ipc::mem::static_alloc, dummy       , 128>::start();
    // test_performance<ipc::mem::static_alloc, alloc_FIFO  , 128>::start();
    // test_performance<ipc::mem::static_alloc, alloc_LIFO  , 128>::start();
    // test_performance<ipc::mem::static_alloc, alloc_random, 128>::start();
}

TEST(Memory, pool_alloc) {
    test_performance<ipc::mem::async_pool_alloc, dummy       , 128>::start();
    test_performance<ipc::mem::async_pool_alloc, alloc_FIFO  , 128>::start();

    test_performance<ipc::mem::async_pool_alloc, dummy       , 128>::start();
    test_performance<ipc::mem::async_pool_alloc, alloc_FIFO  , 128>::start();
    test_performance<ipc::mem::async_pool_alloc, alloc_LIFO  , 128>::start();
    test_performance<ipc::mem::async_pool_alloc, alloc_random, 128>::start();
}

TEST(Memory, tc_alloc) {
    // test_performance<tc_alloc, dummy       , 128>::start();
    // test_performance<tc_alloc, alloc_FIFO  , 128>::start();
    // test_performance<tc_alloc, alloc_LIFO  , 128>::start();
    // test_performance<tc_alloc, alloc_random, 128>::start();
}

} // internal-linkage

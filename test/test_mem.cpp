#include <vector>
#include <array>
#include <thread>
#include <atomic>
#include <cstddef>

#include "capo/random.hpp"

#include "libipc/memory/resource.h"
#include "libipc/pool_alloc.h"

// #include "gperftools/tcmalloc.h"

#include "test.h"
#include "thread_pool.h"

namespace {

constexpr int DataMin   = 4;
constexpr int DataMax   = 256;
constexpr int LoopCount = 8388608;
constexpr int ThreadMax = 8;

// constexpr int DataMin   = 256;
// constexpr int DataMax   = 512;
// constexpr int LoopCount = 2097152;

std::vector<std::size_t> sizes__;
std::vector<void*> ptr_cache__[ThreadMax];

template <typename M>
struct alloc_ix_t {
    static std::vector<int> ix_;
    static bool inited_;

    alloc_ix_t() {
        if (inited_) return;
        inited_ = true;
        M::init();
    }

    template <int ThreadsN>
    static int index(std::size_t /*pid*/, std::size_t /*k*/, std::size_t n) {
        return ix_[n];
    }
};

template <typename M>
std::vector<int> alloc_ix_t<M>::ix_(LoopCount);
template <typename M>
bool alloc_ix_t<M>::inited_ = false;

struct alloc_FIFO : alloc_ix_t<alloc_FIFO> {
    static void init() {
        for (int i = 0; i < LoopCount; ++i) {
            ix_[static_cast<std::size_t>(i)] = i;
        }
    }
};

struct alloc_LIFO : alloc_ix_t<alloc_LIFO> {
    static void init() {
        for (int i = 0; i < LoopCount; ++i) {
            ix_[static_cast<std::size_t>(i)] = i;
        }
    }

    template <int ThreadsN>
    static int index(std::size_t pid, std::size_t k, std::size_t n) {
        constexpr static int CacheSize = LoopCount / ThreadsN;
        if (k) {
            return ix_[(CacheSize * (2 * pid + 1)) - 1 - n];
        }
        else return ix_[n];
    }
};

struct alloc_Random : alloc_ix_t<alloc_Random> {
    static void init() {
        capo::random<> rdm_index(0, LoopCount - 1);
        for (int i = 0; i < LoopCount; ++i) {
            ix_[static_cast<std::size_t>(i)] = rdm_index();
        }
    }
};

struct Init {
    Init() {
        capo::random<> rdm{ DataMin, DataMax };
        for (int i = 0; i < LoopCount; ++i) {
            sizes__.emplace_back(static_cast<std::size_t>(rdm()));
        }
        for (auto& vec : ptr_cache__) {
            vec.resize(LoopCount, nullptr);
        }
    }
} init__;

template <typename AllocT, int ThreadsN>
void benchmark_alloc(char const * message) {
    std::string msg = std::to_string(ThreadsN) + "\t" + message;

    constexpr static int CacheSize = LoopCount / ThreadsN;
    ipc_ut::sender().start(static_cast<std::size_t>(ThreadsN));
    ipc_ut::test_stopwatch sw;

    for (int pid = 0; pid < ThreadsN; ++pid) {
        ipc_ut::sender() << [&, pid] {
            sw.start();
            for (int n = (CacheSize * pid); n < (CacheSize * (pid + 1)); ++n) {
                std::size_t s = sizes__[n];
                AllocT::free(AllocT::alloc(s), s);
            }
        };
    }

    ipc_ut::sender().wait_for_done();
    sw.print_elapsed<1>(DataMin, DataMax, LoopCount, msg.c_str());
}

template <typename AllocT, typename ModeT, int ThreadsN>
void benchmark_alloc(char const * message) {
    std::string msg = std::to_string(ThreadsN) + "\t" + message;

    constexpr static int CacheSize = LoopCount / ThreadsN;
    ModeT mode;
    ipc_ut::sender().start(static_cast<std::size_t>(ThreadsN));
    ipc_ut::test_stopwatch sw;

    for (int pid = 0; pid < ThreadsN; ++pid) {
        ipc_ut::sender() << [&, pid] {
            auto& vec = ptr_cache__[pid];
            sw.start();
            for (std::size_t k = 0; k < 2; ++k)
            for (int n = (CacheSize * pid); n < (CacheSize * (pid + 1)); ++n) {
                int    m = mode.template index<ThreadsN>(pid, k, n);
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
        };
    }

    ipc_ut::sender().wait_for_done();
    sw.print_elapsed<1>(DataMin, DataMax, LoopCount, msg.c_str());
}

template <typename AllocT, typename ModeT, int ThreadsN>
struct test_performance {
    static void start(char const * message) {
        test_performance<AllocT, ModeT, ThreadsN / 2>::start(message);
        benchmark_alloc<AllocT, ModeT, ThreadsN>(message);
    }
};

template <typename AllocT, typename ModeT>
struct test_performance<AllocT, ModeT, 1> {
    static void start(char const * message) {
        benchmark_alloc<AllocT, ModeT, 1>(message);
    }
};

template <typename AllocT, int ThreadsN>
struct test_performance<AllocT, void, ThreadsN> {
    static void start(char const * message) {
        test_performance<AllocT, void, ThreadsN / 2>::start(message);
        benchmark_alloc<AllocT, ThreadsN>(message);
    }
};

template <typename AllocT>
struct test_performance<AllocT, void, 1> {
    static void start(char const * message) {
        benchmark_alloc<AllocT, 1>(message);
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
/*
TEST(Memory, static_alloc) {
    test_performance<ipc::mem::static_alloc, void        , ThreadMax>::start("alloc-free");
    test_performance<ipc::mem::static_alloc, alloc_FIFO  , ThreadMax>::start("alloc-FIFO");
    test_performance<ipc::mem::static_alloc, alloc_LIFO  , ThreadMax>::start("alloc-LIFO");
    test_performance<ipc::mem::static_alloc, alloc_Random, ThreadMax>::start("alloc-Rand");
}

TEST(Memory, pool_alloc) {
    test_performance<ipc::mem::async_pool_alloc, void        , ThreadMax>::start("alloc-free");
    test_performance<ipc::mem::async_pool_alloc, alloc_FIFO  , ThreadMax>::start("alloc-FIFO");
    test_performance<ipc::mem::async_pool_alloc, alloc_LIFO  , ThreadMax>::start("alloc-LIFO");
    test_performance<ipc::mem::async_pool_alloc, alloc_Random, ThreadMax>::start("alloc-Rand");
}
*/
// TEST(Memory, tc_alloc) {
//     test_performance<tc_alloc, void       , ThreadMax>::start();
//     test_performance<tc_alloc, alloc_FIFO  , ThreadMax>::start();
//     test_performance<tc_alloc, alloc_LIFO  , ThreadMax>::start();
//     test_performance<tc_alloc, alloc_Random, ThreadMax>::start();
// }

} // internal-linkage

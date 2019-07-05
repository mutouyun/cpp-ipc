#include <vector>
#include <thread>
#include <atomic>
#include <cstddef>

#include "capo/random.hpp"

#include "memory/resource.h"
#include "pool_alloc.h"

//#include "gperftools/tcmalloc.h"

#include "test.h"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name() const {
        return "test_mem";
    }

private slots:
    void initTestCase();

    void test_alloc_free();
} unit__;

#include "test_mem.moc"

constexpr int DataMin   = 4;
constexpr int DataMax   = 256;
constexpr int LoopCount = 100000;

std::vector<std::size_t> sizes__;

template <typename M>
struct alloc_ix_t {
    static std::vector<int> ix_[2];
    static bool inited_;
};

template <typename M>
std::vector<int> alloc_ix_t<M>::ix_[2] = { std::vector<int>(LoopCount), std::vector<int>(LoopCount) };
template <typename M>
bool alloc_ix_t<M>::inited_ = false;

struct alloc_random : alloc_ix_t<alloc_random> {
    alloc_random() {
        if (inited_) return;
        inited_ = true;
        capo::random<> rdm_index(0, LoopCount - 1);
        for (int i = 0; i < LoopCount; ++i) {
            ix_[0][static_cast<std::size_t>(i)] =
            ix_[1][static_cast<std::size_t>(i)] = rdm_index();
        }
    }
};

struct alloc_LIFO : alloc_ix_t<alloc_LIFO> {
    alloc_LIFO() {
        if (inited_) return;
        inited_ = true;
        for (int i = 0, n = LoopCount - 1; i < LoopCount; ++i, --n) {
            ix_[0][static_cast<std::size_t>(i)] =
            ix_[1][static_cast<std::size_t>(n)] = i;
        }
    }
};

struct alloc_FIFO : alloc_ix_t<alloc_FIFO> {
    alloc_FIFO() {
        if (inited_) return;
        inited_ = true;
        for (int i = 0; i < LoopCount; ++i) {
            ix_[0][static_cast<std::size_t>(i)] =
            ix_[1][static_cast<std::size_t>(i)] = i;
        }
    }
};

void Unit::initTestCase() {
    TestSuite::initTestCase();

    capo::random<> rdm { DataMin, DataMax };
    for (int i = 0; i < LoopCount; ++i) {
        sizes__.emplace_back(static_cast<std::size_t>(rdm()));
    }
}

template <typename AllocT>
void benchmark_alloc() {
    std::cout << std::endl << type_name<AllocT>() << std::endl;

    test_stopwatch sw;
    sw.start();

    for (std::size_t k = 0; k < 100; ++k)
    for (std::size_t n = 0; n < LoopCount; ++n) {
        std::size_t s = sizes__[n];
        AllocT::free(AllocT::alloc(s), s);
    }

    sw.print_elapsed<1>(DataMin, DataMax, LoopCount * 100);
}

template <typename AllocT, typename ModeT, int ThreadsN>
void benchmark_alloc() {
    std::cout << std::endl
              << "[Threads: " << ThreadsN << ", Mode: " << type_name<ModeT>() << "] "
              << type_name<AllocT>() << std::endl;

    std::vector<void*> ptrs[ThreadsN];
    for (auto& vec : ptrs) {
        vec.resize(LoopCount);
    }
    ModeT mode;

    std::atomic_int fini { 0 };
    test_stopwatch  sw;

    std::thread works[ThreadsN];
    int pid = 0;

    for (auto& w : works) {
        w = std::thread {[&, pid] {
            sw.start();
            for (std::size_t k = 0; k < 10; ++k)
            for (std::size_t x = 0; x < 2; ++x) {
                for(std::size_t n = 0; n < LoopCount; ++n) {
                    int    m = mode.ix_[x][n];
                    void*& p = ptrs[pid][static_cast<std::size_t>(m)];
                    std::size_t s  = sizes__[static_cast<std::size_t>(m)];
                    if (p == nullptr) {
                        p = AllocT::alloc(s);
                    }
                    else {
                        AllocT::free(p, s);
                        p = nullptr;
                    }
                }
            }
            if ((fini.fetch_add(1, std::memory_order_relaxed) + 1) == ThreadsN) {
                sw.print_elapsed<1>(DataMin, DataMax, LoopCount * 10 * ThreadsN);
            }
        }};
        ++pid;
    }
    sw.start();

    for (auto& w : works) w.join();
}

template <typename AllocT, typename ModeT, int ThreadsN>
struct test_performance {
    static void start() {
        test_performance<AllocT, ModeT, ThreadsN - 1>::start();
        benchmark_alloc<AllocT, ModeT, ThreadsN>();
    }
};

template <typename AllocT, typename ModeT>
struct test_performance<AllocT, ModeT, 1> {
    static void start() {
        benchmark_alloc<AllocT, ModeT, 1>();
    }
};

//class tc_alloc {
//public:
//    static void clear() {}

//    static void* alloc(std::size_t size) {
//        return size ? tc_malloc(size) : nullptr;
//    }

//    static void free(void* p, std::size_t size) {
//        tc_free_sized(p, size);
//    }
//};

#define TEST_ALLOC_TYPE /*ipc::mem::static_alloc*/ ipc::mem::async_pool_alloc /*tc_alloc*/

void Unit::test_alloc_free() {
//    benchmark_alloc <TEST_ALLOC_TYPE>();
//    test_performance<TEST_ALLOC_TYPE, alloc_FIFO  , 24>::start();

    benchmark_alloc <TEST_ALLOC_TYPE>();
    test_performance<TEST_ALLOC_TYPE, alloc_FIFO  , 16>::start();
    test_performance<TEST_ALLOC_TYPE, alloc_LIFO  , 16>::start();
    test_performance<TEST_ALLOC_TYPE, alloc_random, 16>::start();
}

} // internal-linkage

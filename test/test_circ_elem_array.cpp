#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>
#include <vector>
#include <unordered_map>
#include <functional>

#include "circ_elem_array.h"
#include "test.h"
#include "stopwatch.hpp"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

private slots:
    void test_inst(void);
    void test_prod_cons_1v1(void);
    void test_prod_cons_1v3(void);
    void test_prod_cons_performance(void);
} unit__;

#include "test_circ_elem_array.moc"

using cq_t = ipc::circ::elem_array<12>;
cq_t* cq__;

void Unit::test_inst(void) {
    std::cout << "cq_t::head_size  = " << cq_t::head_size  << std::endl;
    std::cout << "cq_t::data_size  = " << cq_t::data_size  << std::endl;
    std::cout << "cq_t::elem_size  = " << cq_t::elem_size  << std::endl;
    std::cout << "cq_t::block_size = " << cq_t::block_size << std::endl;

    QCOMPARE(static_cast<std::size_t>(cq_t::data_size) , static_cast<std::size_t>(12));
    QCOMPARE(sizeof(cq_t), static_cast<std::size_t>(cq_t::block_size + cq_t::head_size));

    cq__ = new cq_t;
    std::cout << "sizeof(ipc::circ::elem_array<4096>) = " << sizeof(*cq__) << std::endl;

    auto a = cq__->take(1);
    auto b = cq__->take(2);
    QCOMPARE(static_cast<std::size_t>(static_cast<ipc::byte_t*>(b) -
                                      static_cast<ipc::byte_t*>(a)),
             static_cast<std::size_t>(cq_t::elem_size));
}

template <int N, int M, bool Confirmation = true, int Loops = 1000000>
void test_prod_cons(void) {
    ::new (cq__) cq_t;
    std::thread producers[N];
    std::thread consumers[M];
    std::atomic_int fini { 0 };
    capo::stopwatch<> sw;

    struct msg_t {
        int pid_;
        int dat_;
    };

    std::unordered_map<int, std::vector<int>> list[std::extent<decltype(consumers)>::value];
    auto push_data = Confirmation ? [](std::vector<int>& l, int dat) {
        l.push_back(dat);
    } : [](std::vector<int>&, int) {};

    int cid = 0;
    for (auto& t : consumers) {
        t = std::thread{[&, cid] {
            auto cur = cq__->cursor();
            if (Confirmation) {
                std::cout << "start consumer " << &t << ": cur = " << (int)cur << std::endl;
            }

            cq__->connect();
            std::unique_ptr<cq_t, void(*)(cq_t*)> guard(cq__, [](cq_t* cq) { cq->disconnect(); });

            do {
                while (cur != cq__->cursor()) {
                    msg_t* pmsg = static_cast<msg_t*>(cq__->take(cur)),
                            msg = *pmsg;
                    cq__->put(pmsg);
                    if (msg.pid_ < 0) goto finished;
                    ++cur;
                    push_data(list[cid][msg.pid_], msg.dat_);
                }
                std::this_thread::yield();
            } while(1);
        finished:
            if (++fini != std::extent<decltype(consumers)>::value) return;
            auto ts = sw.elapsed<std::chrono::microseconds>();
            std::cout << "[" << N << ":" << M << ", " << Loops << "]" << std::endl
                      << "performance: " << (double(ts) / double(Loops * N)) << " us/d" << std::endl;
            if (!Confirmation) return;
            std::cout << "confirming..." << std::endl;
            for (auto& cons_vec : list) {
                for (int n = 0; n < static_cast<int>(std::extent<decltype(producers)>::value); ++n) {
                    auto& vec = cons_vec[n];
                    QCOMPARE(vec.size(), static_cast<std::size_t>(Loops));
                    int i = 0;
                    for (int d : vec) {
                        QCOMPARE(i, d);
                        ++i;
                    }
                }
            }
        }};
        ++cid;
    }

    while (cq__->conn_count() != std::extent<decltype(consumers)>::value) {
        std::this_thread::yield();
    }

    std::cout << "start producers..." << std::endl;
    std::atomic_flag started = ATOMIC_FLAG_INIT;
    int pid = 0;
    for (auto& t : producers) {
        t = std::thread{[&, pid] {
            if (!started.test_and_set()) {
                sw.start();
            }
            for (int i = 0; i < Loops; ++i) {
                msg_t* pmsg = static_cast<msg_t*>(cq__->acquire());
                pmsg->pid_ = pid;
                pmsg->dat_ = i;
                cq__->commit(pmsg);
            }
        }};
        ++pid;
    }
    for (auto& t : producers) t.join();
    // quit
    msg_t* pmsg = static_cast<msg_t*>(cq__->acquire());
    pmsg->pid_ = pmsg->dat_ = -1;
    cq__->commit(pmsg);

    for (auto& t : consumers) t.join();
}

void Unit::test_prod_cons_1v1(void) {
    test_prod_cons<1, 1>();
}

void Unit::test_prod_cons_1v3(void) {
    test_prod_cons<1, 3>();
}

template <int B, int E>
struct test_performance {
    static void start(void) {
        test_prod_cons<1, B, false>();
        test_performance<B + 1, E>::start();
    }
};

template <int E>
struct test_performance<E, E> {
    static void start(void) {
        test_prod_cons<1, E, false>();
    }
};

void Unit::test_prod_cons_performance(void) {
    test_performance<1, 10>::start();
}

} // internal-linkage

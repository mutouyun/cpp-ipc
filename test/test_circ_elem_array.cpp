#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>
#include <vector>
#include <unordered_map>

#include "circ_elem_array.h"
#include "test.h"
#include "stopwatch.hpp"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

private slots:
    void test_inst(void);
    void test_prod_cons_1v1(void);
    void test_prod_cons_1vN(void);
    void test_prod_cons_Nv1(void);
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
    QCOMPARE(static_cast<std::size_t>(cq_t::block_size), static_cast<std::size_t>(4096));
    QCOMPARE(sizeof(cq_t), static_cast<std::size_t>(cq_t::block_size + cq_t::head_size));

    cq__ = new cq_t;
    std::cout << "sizeof(ipc::circ::elem_array<4096>) = " << sizeof(*cq__) << std::endl;

//    auto a = cq__->take(1);
//    auto b = cq__->take(2);
//    QCOMPARE(static_cast<std::size_t>(static_cast<ipc::byte_t*>(b) -
//                                      static_cast<ipc::byte_t*>(a)),
//             static_cast<std::size_t>(cq_t::elem_size));
}

template <int N, int M, int Loops = 1000000>
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

    for (auto& t : consumers) {
        t = std::thread{[&] {
            auto cur = cq__->cursor();
            std::cout << "start consumer " << &t << ": cur = " << (int)cur << std::endl;

            cq__->connect();
            std::unique_ptr<cq_t, void(*)(cq_t*)> guard(cq__, [](cq_t* cq) { cq->disconnect(); });

            std::unordered_map<int, std::vector<int>> list;
            do {
                while (cur != cq__->cursor()) {
                    msg_t* pmsg = static_cast<msg_t*>(cq__->take(cur)),
                            msg = *pmsg;
                    cq__->put(pmsg);
                    if (msg.pid_ < 0) goto finished;
                    ++cur;
                    list[msg.pid_].push_back(msg.dat_);
                }
            } while(1);
        finished:
            if (++fini == std::extent<decltype(consumers)>::value) {
                auto ts = sw.elapsed<std::chrono::microseconds>();
                std::cout << "[" << N << ":" << M << ", " << Loops << "]" << std::endl
                          << "performance: " << (double(ts) / double(Loops * N)) << " us/d" << std::endl;
            }
            std::cout << "confirming..." << std::endl;
            for (int n = 0; n < static_cast<int>(std::extent<decltype(producers)>::value); ++n) {
                auto& vec = list[n];
                QCOMPARE(vec.size(), static_cast<std::size_t>(Loops));
                int i = 0;
                for (int d : vec) {
                    QCOMPARE(i, d);
                    ++i;
                }
            }
        }};
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
//    test_prod_cons<1, 1>();
}

void Unit::test_prod_cons_1vN(void) {
//    test_prod_cons<1, 3>();
}

void Unit::test_prod_cons_Nv1(void) {
    test_prod_cons<2, 1>();
}

} // internal-linkage

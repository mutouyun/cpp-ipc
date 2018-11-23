#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>
#include <vector>
#include <thread>

#include "circ_elem_array.h"
#include "test.h"
#include "stopwatch.hpp"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

private slots:
    void test_inst(void);
    void test_prod_cons_1vN(void);
} unit__;

#include "test_circ_elem_array.moc"

using cq_t = ipc::circ::elem_array<12>;
cq_t* cq__;

void Unit::test_inst(void) {
    std::cout << "cq_t::head_size  = " << cq_t::head_size  << std::endl;
    std::cout << "cq_t::data_size  = " << cq_t::data_size  << std::endl;
    std::cout << "cq_t::elem_size  = " << cq_t::elem_size  << std::endl;
    std::cout << "cq_t::block_size = " << cq_t::block_size << std::endl;

    QCOMPARE(cq_t::data_size , 12);
    QCOMPARE(cq_t::block_size, 4096);
    QCOMPARE(sizeof(cq_t), cq_t::block_size + cq_t::head_size);

    cq__ = new cq_t;
    std::cout << "sizeof(ipc::circ::elem_array<4096>) = " << sizeof(*cq__) << std::endl;

    auto a = cq__->take(1);
    auto b = cq__->take(2);
    QCOMPARE(static_cast<std::size_t>(static_cast<ipc::byte_t*>(b) -
                                      static_cast<ipc::byte_t*>(a)),
             static_cast<std::size_t>(cq_t::elem_size));
}

void Unit::test_prod_cons_1vN(void) {
    ::new (cq__) cq_t;
    std::thread consumers[1];
    std::atomic_int fini { 0 };
    capo::stopwatch<> sw;
    constexpr static int loops = 1000000;

    for (auto& c : consumers) {
        c = std::thread{[&] {
            auto cur = cq__->cursor();
            std::cout << "start consumer " << &c << ": cur = " << (int)cur << std::endl;

            cq__->connect();
            auto disconn = [](cq_t* cq) { cq->disconnect(); };
            std::unique_ptr<cq_t, decltype(disconn)> guard(cq__, disconn);

            std::vector<int> list;
            int i = 0;
            do {
                while (cur != cq__->cursor()) {
                    auto p = static_cast<int*>(cq__->take(cur));
                    int d = *p;
                    cq__->put(p);
                    if (d < 0) goto finished;
                    ++cur;
                    list.push_back(d);
                }
            } while(1);
        finished:
            if (++fini == std::extent<decltype(consumers)>::value) {
                auto ts = sw.elapsed<std::chrono::microseconds>();
                std::cout << "performance: " << (double(ts) / double(loops)) << " us/d" << std::endl;
            }
            for (int d : list) {
                QCOMPARE(i, d);
                ++i;
            }
        }};
    }

    while (cq__->conn_count() != std::extent<decltype(consumers)>::value) {
        std::this_thread::yield();
    }

    std::cout << "start producer..." << std::endl;
    sw.start();
    for (int i = 0; i < loops; ++i) {
        auto d = static_cast<int*>(cq__->acquire());
        *d = i;
        cq__->commit(d);
    }
    auto d = static_cast<int*>(cq__->acquire());
    *d = -1;
    cq__->commit(d);

    for (auto& c : consumers) {
        c.join();
    }
}

} // internal-linkage

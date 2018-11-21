#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>

#include "circ_queue.h"
#include "test.h"
#include "stopwatch.hpp"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

private slots:
    void test_inst(void);
    void test_producer(void);
} unit__;

#include "test_circ_queue.moc"

using cq_t = ipc::circ_queue<4096>;
cq_t* cq__;

void Unit::test_inst(void) {
    cq__ = new cq_t;
    QCOMPARE(sizeof(*cq__), static_cast<std::size_t>(cq_t::total_size));
    auto a = cq__->get(1);
    auto b = cq__->get(2);
    QCOMPARE(static_cast<std::size_t>(static_cast<std::uint8_t const *>(b) -
                                      static_cast<std::uint8_t const *>(a)),
             static_cast<std::size_t>(cq_t::elem_size));
}

void Unit::test_producer(void) {
    ::new (cq__) cq_t;
    std::thread consumers[1];

    for (auto& c : consumers) {
        c = std::thread{[&c] {
            auto cur = cq__->cursor();
            std::cout << "start consumer " << &c << ": cur = " << (int)cur << std::endl;

            cq__->connect();
            auto disconn = [](cq_t* cq) { cq->disconnect(); };
            std::unique_ptr<cq_t, decltype(disconn)> guard(cq__, disconn);

            auto it = cq__->begin();
            int i = 0;
            do {
                while (it != cq__->end()) {
                    int d = *static_cast<const int*>(cq__->get(cur));
                    if (d < 0) return;
                    QCOMPARE(d, i);
                    ++i; ++it; cq__->next(cur);
                }
            } while(1);
        }};
    }

    while (cq__->conn_count() != std::extent<decltype(consumers)>::value) {
        std::this_thread::yield();
    }
    capo::stopwatch<> sw;
    constexpr static int loops = 267/*1000000*/;

    std::cout << "start producer..." << std::endl;
    sw.start();
    for (int i = 0; i < loops; ++i) {
        auto d = static_cast<int*>(cq__->acquire());
        *d = i;
        cq__->commit();
    }
    auto d = static_cast<int*>(cq__->acquire());
    *d = -1;
    cq__->commit();

    for (auto& c : consumers) {
        c.join();
    }

    auto ts = sw.elapsed<std::chrono::microseconds>();
    std::cout << "time spent : " << (ts / 1000) << " ms" << std::endl;
    std::cout << "performance: " << (double(ts) / double(loops)) << " us/msg" << std::endl;
}

} // internal-linkage

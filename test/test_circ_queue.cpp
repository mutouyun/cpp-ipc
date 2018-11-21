#include <iostream>
#include <string>
#include <type_traits>
#include <memory>

#include "circ_queue.h"
#include "test.h"

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
    QCOMPARE(static_cast<std::size_t>(
             static_cast<std::uint8_t const *>(b) -
             static_cast<std::uint8_t const *>(a)), cq_t::elem_size);
}

void Unit::test_producer(void) {
    cq__ = new cq_t;
    std::thread consumers[3];

    for (auto& c : consumers) {
        c = std::thread{[&c] {
            auto cur = cq__->cursor();
            std::cout << "start consumer " << &c << ": cur = " << (int)cur << std::endl;

            cq__->connect();
            auto disconn = [](cq_t* cq) { cq->disconnect(); };
            std::unique_ptr<cq_t, decltype(disconn)> guard(cq__, disconn);

            int i = 0;
            do {
                while (cur != cq__->cursor()) {
                    int d = *static_cast<const int*>(cq__->get(cur));
//                    std::cout << &c << ": cur = " << (int)cur << ", " << d << std::endl;
                    if (d < 0) {
                        return;
                    }
                    else QCOMPARE(d, i);
                    ++cur;
                    ++i;
                }
            } while(1);
        }};
    }

    while (cq__->conn_count() != std::extent<decltype(consumers)>::value) {
        std::this_thread::yield();
    }
    std::cout << "start producer..." << std::endl;
    for (int i = 0; i < 1000; ++i) {
        auto d = static_cast<int*>(cq__->acquire());
        *d = i;
        cq__->commit();
    }
    auto d = static_cast<int*>(cq__->acquire());
    *d = -1;
    std::cout << "put: quit..." << std::endl;
    cq__->commit();

    for (auto& c : consumers) {
        c.join();
    }
}

} // internal-linkage

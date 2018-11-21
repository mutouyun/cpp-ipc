#include <thread>
#include <iostream>
#include <string>
#include <type_traits>

#include "circ_queue.h"
#include "test.h"

namespace {

using namespace std::string_literals;

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
    std::thread consumers[3];
    std::atomic_int flag(0);

    for (auto& c : consumers) {
        c = std::thread{[&c, &flag] {
            auto cur = cq__->cursor();
            std::cout << &c << ": cur = " << (int)cur << std::endl;
            flag.fetch_add(1, std::memory_order_release);
            do {
                while (cur != cq__->cursor()) {
                    auto data = static_cast<const char*>(cq__->get(cur));
                    std::cout << &c << ": " << data << std::endl;
                    if (data == "quit"s) {
                        return;
                    }
                    else QCOMPARE(data, std::to_string(cur).c_str());
                    ++cur;
                }
                std::this_thread::yield();
            } while(1);
        }};
    }

    while (flag.load(std::memory_order_acquire) != std::extent<decltype(consumers)>::value) {
        std::this_thread::yield();
    }

    for (int i = 0; i < 10; ++i) {
        auto str = static_cast<char*>(cq__->acquire());
        strcpy(str, std::to_string(i).c_str());
        std::cout << "put: " << str << std::endl;
        cq__->commit();
    }
    auto str = static_cast<char*>(cq__->acquire());
    strcpy(str, "quit");
    std::cout << "put: " << str << std::endl;
    cq__->commit();

    for (auto& c : consumers) {
        c.join();
    }
}

} // internal-linkage

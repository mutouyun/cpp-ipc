#include <thread>
#include <iostream>

#include "platform/waiter_wrapper.h"
#include "test.h"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name() const {
        return "test_waiter";
    }

private slots:
    void test_broadcast();
} unit__;

#include "test_waiter.moc"

void Unit::test_broadcast() {
    ipc::detail::waiter w;
    std::thread ts[10];

    for (auto& t : ts) {
        t = std::thread([&w] {
            ipc::detail::waiter_wrapper wp { &w };
            QVERIFY(wp.open("test-ipc-waiter"));
            QVERIFY(wp.wait_if([] { return true; }));
            wp.close();
        });
    }

    ipc::detail::waiter_wrapper wp { &w };
    QVERIFY(wp.open("test-ipc-waiter"));

    std::cout << "waiting for broadcast...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    QVERIFY(wp.broadcast());

    for (auto& t : ts) t.join();
    wp.close();
}

} // internal-linkage

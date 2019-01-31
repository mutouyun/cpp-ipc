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
    void test_multiwait();
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
        });
    }

    ipc::detail::waiter_wrapper wp { &w };
    QVERIFY(wp.open("test-ipc-waiter"));

    std::cout << "waiting for broadcast...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    QVERIFY(wp.broadcast());

    for (auto& t : ts) t.join();
}

void Unit::test_multiwait() {
    ipc::detail::waiter w, mw;
    std::thread ts[10];
    ipc::detail::waiter_wrapper ws[10];

    std::size_t i = 0;
    for (auto& t : ts) {
        t = std::thread([&w, &mw, &ws, i] {
            ipc::detail::waiter_wrapper wp { &w };
            ws[i].attach(&mw);

            QVERIFY(wp.open("test-ipc-waiter"));
            QVERIFY(ws[i].open("test-ipc-multiwait"));

            QVERIFY(wp.wait_if([] { return true; }));
            if (i == 3) {
                std::cout << "waiting for notify...\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                QVERIFY(ws[i].broadcast());
            }
        });
        ++i;
    }

    ipc::detail::waiter_wrapper wp { &w }, wm { &mw };
    QVERIFY(wp.open("test-ipc-waiter"));
    QVERIFY(wm.open("test-ipc-multiwait"));

    std::cout << "waiting for broadcast...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    QVERIFY(wp.broadcast());
    QVERIFY(wm.multi_wait_if(ws, 10, [] { return true; }));

    for (auto& t : ts) t.join();
}

} // internal-linkage

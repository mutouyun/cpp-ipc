#include <thread>

#include "platform/waiter_wrapper.h"
#include "test.h"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name() const {
        return "test_waiter";
    }

private slots:
    void test_wakeup();
} unit__;

#include "test_waiter.moc"

void Unit::test_wakeup() {
    ipc::detail::waiter w;

    std::thread t1 {[&w] {
        ipc::detail::waiter_wrapper wp { &w };
        QVERIFY(wp.open("test-ipc-waiter"));
        QVERIFY(wp.wait());
    }};

    std::thread t2 {[&w] {
        ipc::detail::waiter_wrapper wp { &w };
        QVERIFY(wp.open("test-ipc-waiter"));
        QVERIFY(wp.wait());
    }};

    ipc::detail::waiter_wrapper wp { &w };
    QVERIFY(wp.open("test-ipc-waiter"));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    QVERIFY(wp.broadcast());

    t1.join();
    t2.join();
}

} // internal-linkage

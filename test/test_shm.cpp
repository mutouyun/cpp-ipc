#include <cstring>
#include <cstdint>
#include <thread>

#include "shm.h"
#include "test.h"

using namespace ipc::shm;

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name(void) const {
        return "test_shm";
    }

private slots:
    void test_acquire(void);
    void test_release(void);
    void test_get(void);
    void test_hello(void);
    void test_mt(void);
} unit__;

#include "test_shm.moc"

handle shm_hd__;

void Unit::test_acquire(void) {
    QVERIFY(!shm_hd__.valid());

    QVERIFY(shm_hd__.acquire("my-test-1", 1024));
    QVERIFY(shm_hd__.size() == 1024);
    QCOMPARE(shm_hd__.name(), "my-test-1");

    QVERIFY(shm_hd__.acquire("my-test-2", 2048));
    QVERIFY(shm_hd__.size() == 2048);
    QCOMPARE(shm_hd__.name(), "my-test-2");

    QVERIFY(shm_hd__.acquire("my-test-3", 4096));
    QVERIFY(shm_hd__.size() == 4096);
    QCOMPARE(shm_hd__.name(), "my-test-3");
}

void Unit::test_release(void) {
    QVERIFY(shm_hd__.valid());
    shm_hd__.release();
    QVERIFY(!shm_hd__.valid());
}

void Unit::test_get(void) {
    QVERIFY(shm_hd__.get() == nullptr);
    QVERIFY(shm_hd__.acquire("my-test", 1024));

    auto mem = shm_hd__.get();
    QVERIFY(mem != nullptr);
    QVERIFY(mem == shm_hd__.get());

    std::uint8_t buf[1024];
    memset(buf, 0, sizeof(buf));
    QVERIFY(memcmp(mem, buf, sizeof(buf)) == 0);

    shm_hd__.close();
    QVERIFY(mem == shm_hd__.get());
}

void Unit::test_hello(void) {
    auto mem = shm_hd__.get();
    QVERIFY(mem != nullptr);

    constexpr char hello[] = "hello!";
    std::memcpy(mem, hello, sizeof(hello));
    shm_hd__.close();
    QCOMPARE((char*)shm_hd__.get(), hello);
}

void Unit::test_mt(void) {
    std::thread {
        [] {
            handle shm_mt(shm_hd__.name(), shm_hd__.size());

            shm_hd__.close();
            shm_hd__.release();

            constexpr char hello[] = "hello!";
            QCOMPARE((char*)shm_mt.get(), hello);
        }
    }.join();
    QVERIFY(shm_hd__.get() == nullptr);
    QVERIFY(!shm_hd__.valid());
}

} // internal-linkage

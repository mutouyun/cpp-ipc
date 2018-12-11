#include "ipc.h"
#include "test.h"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name() const {
        return "test_ipc";
    }

private slots:
    void test_send_recv();
    void test_rw_lock();
} unit__;

#include "test_ipc.moc"

void Unit::test_send_recv() {
    auto h = ipc::connect("my-ipc");
    QVERIFY(h != nullptr);
    char data[] = "hello ipc!";
    QVERIFY(ipc::send(h, data, sizeof(data)));
    auto got = ipc::recv(h);
    QCOMPARE((char*)got.data(), data);
    ipc::disconnect(h);
}

void Unit::test_rw_lock() {

}

} // internal-linkage

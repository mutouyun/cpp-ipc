#include <atomic>
#include <iostream>

#include "circ_queue.h"
#include "test.h"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

private slots:
    void test_(void);
} unit__;

#include "test_circ_queue.moc"

void Unit::test_(void) {
    auto* cq = new ipc::circ_queue<4096>;
    QCOMPARE(sizeof(*cq), static_cast<std::size_t>(4096));
}

} // internal-linkage

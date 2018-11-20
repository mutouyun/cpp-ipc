#include <QCoreApplication>
#include <QObject>
#include <QVector>

#include "test.h"

namespace {

QVector<QObject*>* suites__ = nullptr;

} // internal-linkage

TestSuite::TestSuite(void) {
    static struct __ {
        QVector<QObject*> suites_;
        __(void) { suites__ = &suites_; }
    } _;
    _.suites_ << this;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    Q_UNUSED(app)

    int failed_count = 0;
    for (const auto& suite : (*suites__)) {
        if (QTest::qExec(suite, argc, argv) != 0)
            ++failed_count;
    }
    return failed_count;
}

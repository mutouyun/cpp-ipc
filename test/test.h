#pragma once

#include <QtTest>

#include <iostream>

#include "stopwatch.hpp"

class TestSuite : public QObject
{
    Q_OBJECT

public:
    explicit TestSuite();

protected:
    virtual const char* name() const;

protected slots:
    virtual void initTestCase();
};

struct test_stopwatch {
    capo::stopwatch<> sw_;
    std::atomic_flag started_ = ATOMIC_FLAG_INIT;

    void start() {
        if (!started_.test_and_set()) {
            sw_.start();
        }
    }

    void print_elapsed(int N, int M, int Loops) {
        auto ts = sw_.elapsed<std::chrono::microseconds>();
        std::cout << "[" << N << ":" << M << ", " << Loops << "]" << std::endl
                  << "performance: " << (ts / 1000.0) << " ms, "
                  << (double(ts) / double(Loops * N)) << " us/d" << std::endl;
    }
};

#pragma once

#include <QtTest>

#include <iostream>
#include <atomic>
#include <thread>

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

template <typename V>
struct test_verify;

template <>
struct test_verify<void> {
    test_verify   (int)      {}
    void prepare  (void*)    {}
    void verify   (int, int) {}

    template <typename U>
    void push_data(int, U&&) {}
};

template <typename T>
struct test_cq;

template <int N, int M, int Loops, typename V = void, typename T>
void benchmark_prod_cons(T* cq) {
    test_cq<T> tcq { cq };

    std::thread producers[N];
    std::thread consumers[M];
    std::atomic_int fini { 0 };

    test_stopwatch sw;
    test_verify<V> vf { M };

    int cid = 0;
    for (auto& t : consumers) {
        t = std::thread{[&, cid] {
            vf.prepare(&t);
            auto cn = tcq.connect();
            tcq.recv(cn, [&](auto&& msg) { vf.push_data(cid, msg); });
            tcq.disconnect(cn);
            if (++fini != std::extent<decltype(consumers)>::value) return;
            sw.print_elapsed(N, M, Loops);
            vf.verify(N, Loops);
        }};
        ++cid;
    }

    tcq.wait_start(M);

    std::cout << "start producers..." << std::endl;
    int pid = 0;
    for (auto& t : producers) {
        t = std::thread{[&, pid] {
            sw.start();
            for (int i = 0; i < Loops; ++i) {
                tcq.send({ pid, i });
            }
        }};
        ++pid;
    }
    for (auto& t : producers) t.join();
    // quit
    tcq.send({ -1, -1 });

    for (auto& t : consumers) t.join();
}

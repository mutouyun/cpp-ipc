#pragma once

#include <QtTest>

#include <iostream>
#include <atomic>
#include <thread>
#include <string>
#include <cstdio>

#if defined(__GNUC__)
#   include <cxxabi.h>  // abi::__cxa_demangle
#endif/*__GNUC__*/

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
        std::cout << "[" << N << ":" << M << ", " << Loops << "] "
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

template <typename T>
std::string type_name() {
#if defined(__GNUC__)
    const char* typeid_name = typeid(T).name();
    const char* real_name = abi::__cxa_demangle(typeid_name, nullptr, nullptr, nullptr);
    std::unique_ptr<void, decltype(::free)*> guard { (void*)real_name, ::free };
    if (real_name == nullptr) real_name = typeid_name;
    return real_name;
#else
    return typeid(T).name();
#endif/*__GNUC__*/
}

template <int N, int M, int Loops, typename V = void, typename T>
void benchmark_prod_cons(T* cq) {
    std::cout << "benchmark_prod_cons " << type_name<T>() << " [" << N << ":" << M << ", " << Loops << "]" << std::endl;
    test_cq<T> tcq { cq };

    std::thread producers[N];
    std::thread consumers[M];
    std::atomic_int fini_p { 0 }, fini_c { 0 };

    test_stopwatch sw;
    test_verify<V> vf { M };

    int cid = 0;
    for (auto& t : consumers) {
        t = std::thread{[&, cid] {
            vf.prepare(&t);
            auto cn = tcq.connect();
            int i = 0;
            tcq.recv(cn, [&](auto&& msg) {
//                if (i % ((Loops * N) / 10) == 0) {
//                    std::printf("%d-recving: %d%%\n", cid, (i * 100) / (Loops * N));
//                }
                vf.push_data(cid, msg);
                ++i;
            });
//            std::printf("%d-consumer-disconnect\n", cid);
            tcq.disconnect(cn);
            if (++fini_c != std::extent<decltype(consumers)>::value) {
//                std::printf("%d-consumer-end\n", cid);
                return;
            }
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
            auto cn = tcq.connect_send();
            sw.start();
            for (int i = 0; i < Loops; ++i) {
                tcq.send(cn, { pid, i });
//                if (i % (Loops / 10) == 0) {
//                    std::printf("%d-sending: %d%%\n", pid, i * 100 / Loops);
//                }
            }
            if (++fini_p != std::extent<decltype(producers)>::value) return;
            // quit
            tcq.send(cn, { -1, -1 });
        }};
        ++pid;
    }
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}

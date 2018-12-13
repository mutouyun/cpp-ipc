#include <thread>
#include <vector>
#include <type_traits>
#include <iostream>
#include <shared_mutex>
#include <mutex>
#include <typeinfo>
#include <memory>

#if defined(__GNUC__)
#   include <cxxabi.h>  // abi::__cxa_demangle
#endif/*__GNUC__*/

#include "ipc.h"
#include "rw_lock.h"
#include "stopwatch.hpp"
#include "spin_lock.hpp"
#include "test.h"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name() const {
        return "test_ipc";
    }

private slots:
    void test_rw_lock();
    void test_send_recv();
} unit__;

#include "test_ipc.moc"

template <typename T>
constexpr T acc(T b, T e) {
    return (e + b) * (e - b + 1) / 2;
}

template <typename Mutex>
struct lc_wrapper : Mutex {
    void lock_shared  () { Mutex::lock  (); }
    void unlock_shared() { Mutex::unlock(); }
};

template <typename Lc, int W = 4, int R = 4, int Loops = 100000>
void benchmark() {
    std::thread w_trd[W];
    std::thread r_trd[R];
    std::atomic_int fini { 0 };

    std::vector<int> datas;
    Lc lc;

    test_stopwatch sw;
#if defined(__GNUC__)
    {
        const char* typeid_name = typeid(Lc).name();
        const char* real_name = abi::__cxa_demangle(typeid_name, nullptr, nullptr, nullptr);
        std::unique_ptr<void, decltype(::free)*> guard { (void*)real_name, ::free };
        if (real_name == nullptr) real_name = typeid_name;
        std::cout << std::endl << real_name << std::endl;
    }
#else
    std::cout << std::endl << typeid(Lc).name() << std::endl;
#endif/*__GNUC__*/

    for (auto& t : r_trd) {
        t = std::thread([&] {
            std::vector<int> seq;
            std::size_t cnt = 0;
            while (1) {
                int x = -1;
                {
                    std::shared_lock<Lc> guard { lc };
                    if (cnt < datas.size()) {
                        x = datas[cnt];
                    }
                }
                if (x ==  0) break; // quit
                if (x != -1) {
                    seq.push_back(x);
                    ++cnt;
                }
                std::this_thread::yield();
            }
            if (++fini == std::extent<decltype(r_trd)>::value) {
                sw.print_elapsed(W, R, Loops);
            }
            std::int64_t sum = 0;
            for (int i : seq) sum += i;
            QCOMPARE(sum, acc<std::int64_t>(1, Loops) * std::extent<decltype(w_trd)>::value);
        });
    }

    for (auto& t : w_trd) {
        t = std::thread([&] {
            sw.start();
            for (int i = 1; i <= Loops; ++i) {
                {
                    std::unique_lock<Lc> guard { lc };
                    datas.push_back(i);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                std::this_thread::yield();
            }
        });
    }

    for (auto& t : w_trd) t.join();
    lc.lock();
    datas.push_back(0);
    lc.unlock();
    for (auto& t : r_trd) t.join();
}

template <int W, int R>
void test_performance() {

    std::cout << std::endl
              << "test_performance: [" << W << ":" << R << "]"
              << std::endl;

    benchmark<ipc::rw_lock               , W, R>();
//    benchmark<ipc::rw_cas_lock           , W, R>();
//    benchmark<lc_wrapper<capo::spin_lock>, W, R>();
//    benchmark<lc_wrapper<std::mutex>     , W, R>();
//    benchmark<std::shared_timed_mutex    , W, R>();
}

void Unit::test_rw_lock() {
    test_performance<2, 1>();
//    test_performance<4, 4>();
//    test_performance<1, 8>();
//    test_performance<8, 1>();
}

void Unit::test_send_recv() {
//    auto h = ipc::connect("my-ipc");
//    QVERIFY(h != nullptr);
//    char data[] = "hello ipc!";
//    QVERIFY(ipc::send(h, data, sizeof(data)));
//    auto got = ipc::recv(h);
//    QCOMPARE((char*)got.data(), data);
//    ipc::disconnect(h);
}

} // internal-linkage

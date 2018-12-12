#include <thread>
#include <vector>
#include <type_traits>
#include <iostream>
#include <shared_mutex>
#include <mutex>
#include <typeinfo>

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
    void lock_shared  () { lock  (); }
    void unlock_shared() { unlock(); }
};

template <typename Lc, int Loops = 100000, int R = 4, int W = 4>
void benchmark() {
    std::thread r_trd[R];
    std::thread w_trd[W];
    std::atomic_int fini { 0 };

    std::vector<int> datas;
    Lc lc;

    test_stopwatch sw;
    std::cout << std::endl << typeid(Lc).name() << std::endl;

    for (auto& t : r_trd) {
        t = std::thread([&] {
            std::vector<int> seq;
            std::size_t cnt = 0;
            while (1) {
                int x = -1;
                {
                    [[maybe_unused]] std::shared_lock<Lc> guard { lc };
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
                sw.print_elapsed(R, W, Loops);
            }
            std::uint64_t sum = 0;
            for (int i : seq) sum += i;
            QCOMPARE(sum, acc<std::uint64_t>(1, Loops) * std::extent<decltype(w_trd)>::value);
        });
    }

    for (auto& t : w_trd) {
        t = std::thread([&] {
            sw.start();
            for (int i = 1; i <= Loops; ++i) {
                {
                    [[maybe_unused]] std::unique_lock<Lc> guard { lc };
                    datas.push_back(i);
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

void Unit::test_rw_lock() {
    benchmark<ipc::rw_lock>();
    benchmark<lc_wrapper<capo::spin_lock>>();
    benchmark<lc_wrapper<std::mutex>>();
    benchmark<std::shared_mutex>();
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

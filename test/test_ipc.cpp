#include <thread>
#include <vector>
#include <type_traits>
#include <iostream>
#include <shared_mutex>
#include <mutex>
#include <typeinfo>
#include <memory>
#include <string>
#include <cstring>
#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <unordered_map>

#if defined(__GNUC__)
#   include <cxxabi.h>  // abi::__cxa_demangle
#endif/*__GNUC__*/

#include "stopwatch.hpp"
#include "spin_lock.hpp"
#include "random.hpp"

#include "ipc.h"
#include "rw_lock.h"

#include "test.h"

namespace {

std::vector<ipc::buff_t> datas__;

constexpr int DataMin   = 2;
constexpr int DataMax   = 256;
constexpr int LoopCount = 100000;

} // internal-linkage

template <>
struct test_cq<ipc::route> {
    using cn_t = ipc::route;

    std::string conn_name_;

    test_cq(void*)
        : conn_name_("test-ipc-route") {
        ipc::clear_recv(conn_name_.c_str());
    }

    cn_t connect() {
        return { conn_name_.c_str() };
    }

    void disconnect(cn_t& cn) {
        cn.disconnect();
    }

    void wait_start(int M) {
        auto watcher = ipc::connect(conn_name_.c_str());
        while (ipc::recv_count(watcher) != static_cast<std::size_t>(M)) {
            std::this_thread::yield();
        }
    }

    template <typename F>
    void recv(cn_t& cn, F&& proc) {
        do {
            auto msg = cn.recv();
            if (msg.size() < 2) return;
            proc(msg);
        } while(1);
    }

    void send(const std::array<int, 2>& info) {
        thread_local auto cn = connect();
        int n = info[1];
        if (n < 0) {
            cn.send(ipc::buff_t { '\0' });
        }
        else cn.send(datas__[static_cast<decltype(datas__)::size_type>(n)]);
    }
};

template <>
struct test_verify<ipc::route> {
    std::unordered_map<int, std::vector<ipc::buff_t>> list_;
    int lcount_;

    test_verify(int M) : lcount_{ M } {}

    void prepare(void* pt) {
        std::cout << "start consumer: " << pt << std::endl;
    }

    void push_data(int cid, ipc::buff_t const & msg) {
        list_[cid].emplace_back(std::move(msg));
    }

    void verify(int /*N*/, int /*Loops*/) {
        std::cout << "verifying..." << std::endl;
        for (int m = 0; m < lcount_; ++m) {
            QCOMPARE(datas__, list_[m]);
        }
    }
};

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name() const {
        return "test_ipc";
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_rw_lock();
    void test_send_recv();
    void test_route();
    void test_route_performance();
    void test_channel();
} unit__;

#include "test_ipc.moc"

void Unit::initTestCase() {
    TestSuite::initTestCase();

    capo::random<> rdm { DataMin, DataMax };
    capo::random<> bit { 0, (std::numeric_limits<ipc::buff_t::value_type>::max)() };

    for (int i = 0; i < LoopCount; ++i) {
        auto n = rdm();
        ipc::buff_t buff(static_cast<ipc::buff_t::size_type>(n));
        for (std::size_t k = 0; k < buff.size(); ++k) {
            buff[k] = static_cast<ipc::buff_t::value_type>(bit());
        }
        datas__.emplace_back(std::move(buff));
    }
}

void Unit::cleanupTestCase() {
    datas__.clear();
}

template <typename T>
constexpr T acc(T b, T e) {
    return (e + b) * (e - b + 1) / 2;
}

template <typename Mutex>
struct lc_wrapper : Mutex {
    void lock_shared  () { Mutex::lock  (); }
    void unlock_shared() { Mutex::unlock(); }
};

template <typename Lc, int W, int R, int Loops = 100000>
void benchmark_lc() {
    std::thread w_trd[W];
    std::thread r_trd[R];
    std::atomic_int fini { 0 };
//    std::atomic_bool wf { false };

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
//                    QVERIFY(!wf);
                    if (cnt < datas.size()) {
                        x = datas[cnt];
                    }
//                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if (x ==  0) break; // quit
                    if (x != -1) {
                        seq.push_back(x);
                        ++cnt;
                    }
                }
                std::this_thread::yield();
            }
            if (++fini == std::extent<decltype(r_trd)>::value) {
                sw.print_elapsed(W, R, Loops);
            }
            std::uint64_t sum = 0;
            for (int i : seq) sum += static_cast<std::uint64_t>(i);
            QCOMPARE(sum, acc<std::uint64_t>(1, Loops) * std::extent<decltype(w_trd)>::value);
        });
    }

    for (auto& t : w_trd) {
        t = std::thread([&] {
            sw.start();
            for (int i = 1; i <= Loops; ++i) {
                {
                    std::unique_lock<Lc> guard { lc };
//                    wf = true;
                    datas.push_back(i);
//                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
//                    wf = false;
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
void test_lock_performance() {

    std::cout << std::endl
              << "test_lock_performance: [" << W << ":" << R << "]"
              << std::endl;

    benchmark_lc<ipc::rw_lock               , W, R>();
    benchmark_lc<lc_wrapper<capo::spin_lock>, W, R>();
    benchmark_lc<lc_wrapper<std::mutex>     , W, R>();
    benchmark_lc<std::shared_timed_mutex    , W, R>();
}

void Unit::test_rw_lock() {
    test_lock_performance<1, 1>();
    test_lock_performance<4, 4>();
    test_lock_performance<1, 8>();
    test_lock_performance<8, 1>();
}

void Unit::test_send_recv() {
    auto h = ipc::connect("my-ipc");
    QVERIFY(h != nullptr);
    ipc::clear_recv(h);
    char data[] = "hello ipc!";
    QVERIFY(ipc::send(h, data, sizeof(data)));
    auto got = ipc::recv(h);
    QCOMPARE((char*)got.data(), data);
    ipc::disconnect(h);
}

void Unit::test_route() {
    ipc::clear_recv("my-ipc-route");

    auto wait_for_handshake = [](int id) {
        ipc::route cc { "my-ipc-route" };
        std::string cfm = "copy:" + std::to_string(id), ack = "re-" + cfm;
        std::atomic_bool unmatched { true };
        std::thread re {[&] {
            bool has_re = false;
            do {
                auto dd = cc.recv();
                QVERIFY(!dd.empty());
                std::string got { reinterpret_cast<char*>(dd.data()), dd.size() - 1 };
                if (cfm == got) continue;
                std::cout << id << "-recv: " << got << "[" << dd.size() << "]" << std::endl;
                if (ack != got) {
                    char const cp[] = "copy:";
                    // check header
                    if (std::memcmp(dd.data(), cp, sizeof(cp) - 1) == 0) {
                        std::cout << id << "-re: " << got << std::endl;
                        QVERIFY(has_re = cc.send(
                                    std::string{ "re-" }.append(
                                        reinterpret_cast<char*>(dd.data()), dd.size() - 1)));
                    }
                }
                else if (unmatched.load(std::memory_order_relaxed)) {
                    unmatched.store(false, std::memory_order_release);
                    std::cout << id << "-matched!" << std::endl;
                }
            } while (!has_re || unmatched.load(std::memory_order_relaxed));
        }};
        while (unmatched.load(std::memory_order_acquire)) {
            if (!cc.send(cfm)) {
                std::cout << id << "-send failed!" << std::endl;
                unmatched = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        re.join();
        std::cout << id << "-fini handshake!" << std::endl;
        return cc;
    };

    std::vector<char const *> const datas = {
        "hello!",
        "foo",
        "bar",
        "ISO/IEC",
        "14882:2011",
        "ISO/IEC 14882:2017 Information technology - Programming languages - C++",
        "ISO/IEC 14882:2020",
        "Modern C++ Design: Generic Programming and Design Patterns Applied"
    };

    std::thread t1 {[&] {
        auto cc = wait_for_handshake(1);
        const char cp[] = "copy:", re[] = "re-copy:";
        bool unchecked = true;
        for (std::size_t i = 0; i < datas.size(); ++i, unchecked = false) {
            ipc::buff_t dd;
            do {
                dd = cc.recv();
            } while (unchecked &&
                     (((dd.size() > sizeof(cp)) && std::memcmp(dd.data(), cp, sizeof(cp) - 1) == 0) ||
                      ((dd.size() > sizeof(re)) && std::memcmp(dd.data(), re, sizeof(re) - 1) == 0)));
            QCOMPARE(dd.size(), std::strlen(datas[i]) + 1);
            QVERIFY(std::memcmp(dd.data(), datas[i], dd.size()) == 0);
        }
    }};

    std::thread t2 {[&] {
        auto cc = wait_for_handshake(2);
        for (std::size_t i = 0; i < datas.size(); ++i) {
            std::cout << "sending: " << datas[i] << std::endl;
            cc.send(datas[i]);
        }
    }};

    t1.join();
    t2.join();
}

template <int N, int M, bool V = true, int Loops = LoopCount>
void test_prod_cons() {
    benchmark_prod_cons<N, M, Loops, std::conditional_t<V, ipc::route, void>>((ipc::route*)nullptr);
}

template <int P, int C>
struct test_performance {
    static void start() {
        test_performance<P - 1, C - 1>::start();
        test_prod_cons<P, C, false>();
    }
};

template <int C>
struct test_performance<1, C> {
    static void start() {
        test_performance<1, C - 1>::start();
        test_prod_cons<1, C, false>();
    }
};

template <int P>
struct test_performance<P, 1> {
    static void start() {
        test_performance<P - 1, 1>::start();
        test_prod_cons<P, 1, false>();
    }
};

template <>
struct test_performance<1, 1> {
    static void start() {
        test_prod_cons<1, 1, false>();
    }
};

void Unit::test_route_performance() {
    test_prod_cons<1, 1>();
    test_performance<1, 10>::start();
}

void Unit::test_channel() {
    std::thread t1 {[&] {
        ipc::channel cc { "my-ipc-channel" };
        for (std::size_t i = 0;; ++i) {
            ipc::buff_t dd = cc.recv();
            if (dd.size() < 2) return;
            QCOMPARE(dd, datas__[i]);
        }
    }};

    std::thread t2 {[&] {
        ipc::channel cc { "my-ipc-channel" };
        while (cc.recv_count() == 0) {
            std::this_thread::yield();
        }
        for (std::size_t i = 0; i < (std::min)(100, LoopCount); ++i) {
            std::cout << "sending: " << i << "-[" << datas__[i].size() << "]" << std::endl;
            cc.send(datas__[i]);
        }
        cc.send(ipc::buff_t { '\0' });
        t1.join();
    }};

    t2.join();
}

} // internal-linkage

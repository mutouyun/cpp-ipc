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

#if defined(__GNUC__)
#   include <cxxabi.h>  // abi::__cxa_demangle
#endif/*__GNUC__*/

#include "ipc.h"
#include "rw_lock.h"
#include "stopwatch.hpp"
#include "spin_lock.hpp"
#include "random.hpp"
#include "test.h"

namespace {

std::vector<ipc::buff_t> datas__;

constexpr int DataMin   = 2;
constexpr int DataMax   = 512;
constexpr int LoopCount = 100/*0*//*000*/;

} // internal-linkage

template <>
struct test_cq<ipc::channel> {
    using cn_t = ipc::channel;

    std::string conn_name_;
    std::size_t conn_count_ = 0;

    test_cq(void*)
        : conn_name_("test-ipc-channel")
    {}

    cn_t connect() {
        cn_t cn { conn_name_.c_str() };
        conn_count_ = cn.conn_count();
        return cn;
    }

    void disconnect(cn_t&) {}

    void wait_start(int M) {
        while (conn_count_ != static_cast<std::size_t>(M)) {
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
        else cn.send(datas__[n]);
    }
};

//template <>
//struct test_verify<true> {
//    std::unordered_map<int, std::vector<ipc::buff_t>> list_;
//    int lcount_;

//    test_verify(int M) : lcount_{ M } {}

//    void prepare(void* pt) {
//        std::cout << "start consumer: " << pt << std::endl;
//    }

//    void push_data(int cid, ipc::buff_t const & msg) {
//        list_[cid].emplace_back(std::move(msg));
//    }

//    void verify(int /*N*/, int /*Loops*/) {
//        std::cout << "verifying..." << std::endl;
//        for (int m = 0; m < lcount_; ++m) {
//            QCOMPARE(datas__, list_[m]);
//        }
//    }
//};

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
    void test_channel();
    void test_channel_performance();
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
            for (int i : seq) sum += i;
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
void test_performance() {

    std::cout << std::endl
              << "test_performance: [" << W << ":" << R << "]"
              << std::endl;

    benchmark_lc<ipc::rw_lock               , W, R>();
    benchmark_lc<lc_wrapper<capo::spin_lock>, W, R>();
    benchmark_lc<lc_wrapper<std::mutex>     , W, R>();
    benchmark_lc<std::shared_timed_mutex    , W, R>();
}

void Unit::test_rw_lock() {
    test_performance<1, 1>();
    test_performance<4, 4>();
    test_performance<1, 8>();
    test_performance<8, 1>();
}

void Unit::test_send_recv() {
    auto h = ipc::connect("my-ipc");
    QVERIFY(h != nullptr);
    char data[] = "hello ipc!";
    QVERIFY(ipc::send(h, data, sizeof(data)));
    auto got = ipc::recv(h);
    QCOMPARE((char*)got.data(), data);
    ipc::disconnect(h);
}

void Unit::test_channel() {
    auto wait_for_handshake = [](int id) {
        ipc::channel cc { "my-ipc-channel" };
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

void Unit::test_channel_performance() {
    benchmark_prod_cons<1, 1, LoopCount, false>((ipc::channel*)nullptr);
}

} // internal-linkage

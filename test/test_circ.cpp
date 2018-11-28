#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>
#include <vector>
#include <unordered_map>
#include <functional>

#include "circ_elem_array.h"
#include "circ_queue.h"
#include "test.h"
#include "stopwatch.hpp"

namespace {

class Unit : public TestSuite {
    Q_OBJECT

private slots:
    void initTestCase(void);
    void cleanupTestCase(void);

    void test_inst(void);
    void test_prod_cons_1v1(void);
    void test_prod_cons_1v3(void);
    void test_prod_cons_performance(void);

    void test_queue(void);
} unit__;

#include "test_circ.moc"

using cq_t = ipc::circ::elem_array<12>;
cq_t* cq__;

void Unit::initTestCase(void) {
    cq__ = new cq_t;
}

void Unit::cleanupTestCase(void) {
    delete cq__;
}

void Unit::test_inst(void) {
    std::cout << "cq_t::head_size  = " << cq_t::head_size  << std::endl;
    std::cout << "cq_t::data_size  = " << cq_t::data_size  << std::endl;
    std::cout << "cq_t::elem_size  = " << cq_t::elem_size  << std::endl;
    std::cout << "cq_t::block_size = " << cq_t::block_size << std::endl;

    QCOMPARE(static_cast<std::size_t>(cq_t::data_size) , static_cast<std::size_t>(12));
    QCOMPARE(sizeof(cq_t), static_cast<std::size_t>(cq_t::block_size + cq_t::head_size));

    std::cout << "sizeof(ipc::circ::elem_array<4096>) = " << sizeof(*cq__) << std::endl;

    auto a = cq__->take(1);
    auto b = cq__->take(2);
    QCOMPARE(static_cast<std::size_t>(static_cast<ipc::byte_t*>(b) -
                                      static_cast<ipc::byte_t*>(a)),
             static_cast<std::size_t>(cq_t::elem_size));
}

struct msg_t {
    int pid_;
    int dat_;
};

struct test_stopwatch {
    capo::stopwatch<> sw_;
    std::atomic_flag started_ = ATOMIC_FLAG_INIT;

    void start(void) {
        if (!started_.test_and_set()) {
            sw_.start();
        }
    }

    void print_elapsed(int N, int M, int Loops) {
        auto ts = sw_.elapsed<std::chrono::microseconds>();
        std::cout << "[" << N << ":" << M << ", " << Loops << "]" << std::endl
                  << "performance: " << (double(ts) / double(Loops * N)) << " us/d" << std::endl;
    }
};

template <bool Confirmation>
struct test_confirm {
    std::unordered_map<int, std::vector<int>>* list_;
    int lcount_;

    test_confirm(int M) {
        list_ = new std::remove_reference_t<decltype(*list_)>[
                    static_cast<std::size_t>(lcount_ = M)
                ];
    }

    ~test_confirm(void) {
        delete [] list_;
    }

    void prepare(void* pt) {
        std::cout << "start consumer: " << pt << std::endl;
    }

    void push_data(int cid, msg_t const & msg) {
        list_[cid][msg.pid_].push_back(msg.dat_);
    }

    void verify(int N, int Loops) {
        std::cout << "confirming..." << std::endl;
        for (int m = 0; m < lcount_; ++m) {
            auto& cons_vec = list_[m];
            for (int n = 0; n < N; ++n) {
                auto& vec = cons_vec[n];
                QCOMPARE(vec.size(), static_cast<std::size_t>(Loops));
                int i = 0;
                for (int d : vec) {
                    QCOMPARE(i, d);
                    ++i;
                }
            }
        }
    }
};

template <>
struct test_confirm<false> {
    test_confirm  (int)                {}
    void prepare  (void*)              {}
    void push_data(int, msg_t const &) {}
    void verify   (int, int)           {}
};

template <typename T>
struct test_cq;

template <std::size_t D>
struct test_cq<ipc::circ::elem_array<D>> {
    using ca_t = ipc::circ::elem_array<D>;
    using cn_t = typename std::result_of<decltype(&ca_t::cursor)(ca_t)>::type;

    ca_t* ca_;

    test_cq(ca_t* ca) : ca_(ca) {
        ::new (ca) ca_t;
    }

    cn_t connect(void) {
        auto cur = ca_->cursor();
        ca_->connect();
        return cur;
    }

    void disconnect(cn_t) {
        ca_->disconnect();
    }

    void wait_start(int M) {
        while (ca_->conn_count() != static_cast<std::size_t>(M)) {
            std::this_thread::yield();
        }
    }

    template <typename F>
    void recv(cn_t cur, F&& proc) {
        do {
            while (cur != ca_->cursor()) {
                msg_t* pmsg = static_cast<msg_t*>(ca_->take(cur)),
                        msg = *pmsg;
                ca_->put(pmsg);
                if (msg.pid_ < 0) return;
                ++cur;
                proc(msg);
            }
            std::this_thread::yield();
        } while(1);
    }

    void send(msg_t const & msg) {
        msg_t* pmsg = static_cast<msg_t*>(ca_->acquire());
        (*pmsg) = msg;
        ca_->commit(pmsg);
    }
};

template <typename T>
struct test_cq<ipc::circ::queue<T>> {
    using cn_t = ipc::circ::queue<T>;
    using ca_t = typename cn_t::array_t;

    ca_t* ca_;

    test_cq(void*) : ca_(reinterpret_cast<ca_t*>(cq__)) {
        ::new (ca_) ca_t;
    }

    cn_t connect(void) {
        cn_t queue;
        [&] {
            queue.attch(ca_);
            QVERIFY(queue.connect() != ipc::error_count);
        } ();
        return queue;
    }

    void disconnect(cn_t& queue) {
        QVERIFY(queue.disconnect() != ipc::error_count);
        QVERIFY(queue.detach() != nullptr);
    }

    void wait_start(int M) {
        while (ca_->conn_count() != static_cast<std::size_t>(M)) {
            std::this_thread::yield();
        }
    }

    template <typename F>
    void recv(cn_t& queue, F&& proc) {
        do {
            auto msg = queue.pop();
            if (msg.pid_ < 0) return;
            proc(msg);
        } while(1);
    }

    void send(msg_t const & msg) {
        cn_t{ ca_ }.push(msg);
    }
};

template <int N, int M, bool C = true, int Loops = 1000000, typename T>
void test_prod_cons(T* cq) {
    test_cq<T> tcq { cq };

    std::thread producers[N];
    std::thread consumers[M];
    std::atomic_int fini { 0 };

    test_stopwatch  sw;
    test_confirm<C> cf { M };

    int cid = 0;
    for (auto& t : consumers) {
        t = std::thread{[&, cid] {
            cf.prepare(&t);
            auto cn = tcq.connect();

            using namespace std::placeholders;
            tcq.recv(cn, std::bind(&test_confirm<C>::push_data, &cf, cid, _1));

            tcq.disconnect(cn);
            if (++fini != std::extent<decltype(consumers)>::value) return;
            sw.print_elapsed(N, M, Loops);
            cf.verify(N, Loops);
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

template <int N, int M, bool C = true, int Loops = 1000000>
void test_prod_cons(void) {
    test_prod_cons<N, M, C, Loops>(cq__);
}

void Unit::test_prod_cons_1v1(void) {
    test_prod_cons<1, 1>();
}

void Unit::test_prod_cons_1v3(void) {
    test_prod_cons<1, 3>();
}

template <int B, int E>
struct test_performance {
    static void start(void) {
        test_prod_cons<1, B, false>();
        test_performance<B + 1, E>::start();
    }
};

template <int E>
struct test_performance<E, E> {
    static void start(void) {
        test_prod_cons<1, E, false>();
    }
};

void Unit::test_prod_cons_performance(void) {
    test_performance<1, 10>::start();
}

void Unit::test_queue(void) {
    ipc::circ::queue<msg_t> queue;
    queue.push(1, 2);
    QVERIFY_EXCEPTION_THROWN(queue.pop(), std::exception);
    QVERIFY(sizeof(decltype(queue)::array_t) <= sizeof(*cq__));

    auto cq = ::new (cq__) decltype(queue)::array_t;
    queue.attch(cq);
    QVERIFY(queue.detach() != nullptr);

    test_prod_cons<1, 3>((ipc::circ::queue<msg_t>*)nullptr);
}

} // internal-linkage

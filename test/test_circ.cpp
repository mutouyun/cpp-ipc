#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>
#include <vector>
#include <unordered_map>

#include "circ_elems_array.h"
#include "circ_elem_array.h"
#include "circ_queue.h"
#include "memory/resource.hpp"
#include "test.h"

namespace {

struct msg_t {
    int pid_;
    int dat_;
};

using cq_t = ipc::circ::elem_array<sizeof(msg_t)>;
cq_t* cq__;

bool operator==(msg_t const & m1, msg_t const & m2) {
    return (m1.pid_ == m2.pid_) && (m1.dat_ == m2.dat_);
}

} // internal-linkage

template <>
struct test_verify<cq_t> {
    std::vector<std::unordered_map<int, std::vector<int>>> list_;

    test_verify(int M)
        : list_(static_cast<std::size_t>(M))
    {}

    void prepare(void* pt) {
        std::cout << "start consumer: " << pt << std::endl;
    }

    void push_data(int cid, msg_t const & msg) {
        list_[cid][msg.pid_].push_back(msg.dat_);
    }

    void verify(int N, int Loops) {
        std::cout << "verifying..." << std::endl;
        for (auto& c_dats : list_) {
            for (int n = 0; n < N; ++n) {
                auto& vec = c_dats[n];
                //for (int d : vec) {
                //    std::cout << d << " ";
                //}
                //std::cout << std::endl;
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
struct test_verify<ipc::circ::prod_cons<
        ipc::circ::relat::single,
        ipc::circ::relat::multi,
        ipc::circ::trans::unicast>
    > : test_verify<cq_t> {
    using test_verify<cq_t>::test_verify;

    void verify(int N, int Loops) {
        std::cout << "verifying..." << std::endl;
        for (int n = 0; n < N; ++n) {
            std::vector<int> datas;
            std::uint64_t sum = 0;
            for (auto& c_dats : list_) {
                for (int d : c_dats[n]) {
                    datas.push_back(d);
                    sum += d;
                }
            }
            QCOMPARE(datas.size(), static_cast<std::size_t>(Loops));
            QCOMPARE(sum, (Loops * std::uint64_t(Loops - 1)) / 2);
        }
    }
};

template <typename P>
struct quit_mode;

template <ipc::circ::relat Rp, ipc::circ::relat Rc>
struct quit_mode<ipc::circ::prod_cons<Rp, Rc, ipc::circ::trans::unicast>> {
    using type = volatile bool;
};

template <ipc::circ::relat Rp, ipc::circ::relat Rc>
struct quit_mode<ipc::circ::prod_cons<Rp, Rc, ipc::circ::trans::broadcast>> {
    struct type {
        type(bool) {}
        constexpr operator bool() const { return false; }
    };
};

template <std::size_t D, typename P>
struct test_cq<ipc::circ::elems_array<D, P>> {
    using ca_t = ipc::circ::elems_array<D, P>;
    using cn_t = decltype(std::declval<ca_t>().cursor());

    typename quit_mode<P>::type quit_ = false;
    ca_t* ca_;

    test_cq(ca_t* ca) : ca_(ca) {}

    cn_t connect() {
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
        while(1) {
            msg_t msg;
            while (ca_->pop(cur, [&msg](void* p) {
                msg = *static_cast<msg_t*>(p);
            })) {
                if (msg.pid_ < 0) {
                    quit_ = true;
                    return;
                }
                proc(msg);
            }
            if (quit_) return;
            std::this_thread::yield();
        }
    }

    ca_t* connect_send() {
        return ca_;
    }

    void send(ca_t* ca, msg_t const & msg) {
        while (!ca->push([&msg](void* p) {
            (*static_cast<msg_t*>(p)) = msg;
        })) {
            std::this_thread::yield();
        }
    }
};

template <std::size_t D>
struct test_cq<ipc::circ::elem_array<D>> {
    using ca_t = ipc::circ::elem_array<D>;
    using cn_t = typename ca_t::u2_t;

    ca_t* ca_;

    test_cq(ca_t* ca) : ca_(ca) {
        ::new (ca) ca_t;
    }

    cn_t connect() {
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
        while(1) {
            while (cur != ca_->cursor()) {
                msg_t* pmsg = static_cast<msg_t*>(ca_->take(cur)),
                        msg = *pmsg;
                ca_->put(pmsg);
                if (msg.pid_ < 0) return;
                ++cur;
                proc(msg);
            }
            std::this_thread::yield();
        }
    }

    ca_t* connect_send() {
        return ca_;
    }

    void send(ca_t* ca, msg_t const & msg) {
        msg_t* pmsg = static_cast<msg_t*>(ca->acquire());
        (*pmsg) = msg;
        ca->commit(pmsg);
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

    cn_t* connect() {
        cn_t* queue = new cn_t { ca_ };
        [&] { QVERIFY(queue->connect() != ipc::invalid_value); } ();
        return queue;
    }

    void disconnect(cn_t* queue) {
        QVERIFY(queue->disconnect() != ipc::invalid_value);
        QVERIFY(queue->detach() != nullptr);
        delete queue;
    }

    void wait_start(int M) {
        while (ca_->conn_count() != static_cast<std::size_t>(M)) {
            std::this_thread::yield();
        }
    }

    template <typename F>
    void recv(cn_t* queue, F&& proc) {
        while(1) {
            auto msg = queue->pop();
            if (msg.pid_ < 0) return;
            proc(msg);
        }
    }

    cn_t connect_send() {
        return cn_t{ ca_ };
    }

    void send(cn_t& cn, msg_t const & msg) {
        cn.push(msg);
    }
};

namespace {

class Unit : public TestSuite {
    Q_OBJECT

    const char* name() const {
        return "test_circ";
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_inst();
    void test_prod_cons_1v1();
    void test_prod_cons_1v3();
    void test_prod_cons_performance();
    void test_queue();
} unit__;

#include "test_circ.moc"

constexpr int LoopCount = 10000000;
//constexpr int LoopCount = 1000/*0000*/;

void Unit::initTestCase() {
    TestSuite::initTestCase();
    cq__ = new cq_t;
}

void Unit::cleanupTestCase() {
    delete cq__;
}

void Unit::test_inst() {
    std::cout << "cq_t::head_size  = " << cq_t::head_size  << std::endl;
    std::cout << "cq_t::data_size  = " << cq_t::data_size  << std::endl;
    std::cout << "cq_t::elem_size  = " << cq_t::elem_size  << std::endl;
    std::cout << "cq_t::block_size = " << cq_t::block_size << std::endl;

    QCOMPARE(static_cast<std::size_t>(cq_t::data_size), sizeof(msg_t));
    QCOMPARE(sizeof(cq_t), static_cast<std::size_t>(cq_t::block_size + cq_t::head_size));

    std::cout << "sizeof(ipc::circ::elem_array<4096>) = " << sizeof(*cq__) << std::endl;

    auto a = cq__->take(1);
    auto b = cq__->take(2);
    QCOMPARE(static_cast<std::size_t>(static_cast<ipc::byte_t*>(b) -
                                      static_cast<ipc::byte_t*>(a)),
             static_cast<std::size_t>(cq_t::elem_size));
}

template <int N, int M, bool V = true, int Loops = LoopCount>
void test_prod_cons() {
    benchmark_prod_cons<N, M, Loops, std::conditional_t<V, cq_t, void>>(cq__);
}

void Unit::test_prod_cons_1v1() {
    test_prod_cons<1, 1>();

    ipc::circ::elems_array<
        sizeof(msg_t),
        ipc::circ::prod_cons<ipc::circ::relat::single,
                             ipc::circ::relat::single,
                             ipc::circ::trans::unicast>
    > el_arr_ss;
    benchmark_prod_cons<1, 1, LoopCount, cq_t>(&el_arr_ss);
    benchmark_prod_cons<1, 1, LoopCount, void>(&el_arr_ss);
}

void Unit::test_prod_cons_1v3() {
    test_prod_cons<1, 3>();

    ipc::circ::elems_array<
        sizeof(msg_t),
        ipc::circ::prod_cons<ipc::circ::relat::single,
                             ipc::circ::relat::multi,
                             ipc::circ::trans::unicast>
    > el_arr_smn;
    benchmark_prod_cons<1, 3, LoopCount, decltype(el_arr_smn)::policy_t>(&el_arr_smn);
    benchmark_prod_cons<1, 3, LoopCount, void>(&el_arr_smn);

    ipc::circ::elems_array<
        sizeof(msg_t),
        ipc::circ::prod_cons<ipc::circ::relat::single,
                             ipc::circ::relat::multi,
                             ipc::circ::trans::broadcast>
    > el_arr_smm;
    benchmark_prod_cons<1, 3, LoopCount, cq_t>(&el_arr_smm);
    benchmark_prod_cons<1, 3, LoopCount, void>(&el_arr_smm);
}

void Unit::test_prod_cons_performance() {
    ipc::mem::detail::static_for(std::make_index_sequence<10>{}, [](auto index) {
        test_prod_cons<1, decltype(index)::value + 1, false>();
    });
    test_prod_cons<1, 10>(); // test & verify
}

void Unit::test_queue() {
    ipc::circ::queue<msg_t> queue;
    queue.push(msg_t { 1, 2 });
    QCOMPARE(queue.pop(), msg_t{});
    QVERIFY(sizeof(decltype(queue)::array_t) <= sizeof(*cq__));

    auto cq = ::new (cq__) decltype(queue)::array_t;
    queue.attach(cq);
    QVERIFY(queue.detach() != nullptr);

    ipc::mem::detail::static_for(std::make_index_sequence<10>{}, [](auto index) {
        benchmark_prod_cons<1, decltype(index)::value + 1, LoopCount>((ipc::circ::queue<msg_t>*)nullptr);
    });
}

} // internal-linkage

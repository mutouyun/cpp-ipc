#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>
#include <vector>
#include <unordered_map>

#include "queue.h"
#include "prod_cons.h"
#include "policy.h"
#include "circ/elem_array.h"
#include "memory/resource.h"
#include "test.h"

namespace {

struct msg_t {
    int pid_;
    int dat_;
};

template <ipc::relat Rp, ipc::relat Rc, ipc::trans Ts>
using pc_t = ipc::prod_cons_impl<ipc::wr<Rp, Rc, Ts>>;

template <std::size_t DataSize, typename Policy>
struct ea_t : public ipc::circ::elem_array<Policy, DataSize, 1> {
    ea_t() { std::memset(this, 0, sizeof(ipc::circ::elem_array<Policy, DataSize, 1>)); }
};

using cq_t = ea_t<
    sizeof(msg_t),
    pc_t<ipc::relat::single, ipc::relat::multi, ipc::trans::broadcast>
>;
cq_t* cq__;

bool operator==(msg_t const & m1, msg_t const & m2) {
    return (m1.pid_ == m2.pid_) && (m1.dat_ == m2.dat_);
}

} // internal-linkage

template <std::size_t D, typename P>
struct test_verify<ea_t<D, P>> {
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

template <ipc::relat Rp>
struct test_verify<pc_t<Rp, ipc::relat::multi, ipc::trans::unicast>> : test_verify<cq_t> {
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

template <ipc::relat Rp, ipc::relat Rc>
struct quit_mode<pc_t<Rp, Rc, ipc::trans::unicast>> {
    using type = volatile bool;
};

template <ipc::relat Rp, ipc::relat Rc>
struct quit_mode<pc_t<Rp, Rc, ipc::trans::broadcast>> {
    struct type {
        constexpr type(bool) {}
        constexpr operator bool() const { return false; }
    };
};

template <std::size_t D, typename P>
struct test_cq<ea_t<D, P>> {
    using ca_t = ea_t<D, P>;
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

    void disconnect(ca_t*) {
    }

    void wait_start(int M) {
        while (ca_->conn_count() != static_cast<std::size_t>(M)) {
            std::this_thread::yield();
        }
    }

    template <typename F>
    void recv(cn_t cur, F&& proc) {
        while (1) {
            msg_t msg;
            while (ca_->pop(&cur, [&msg](void* p) {
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

template <typename... T>
struct test_cq<ipc::queue<T...>> {
    using cn_t = ipc::queue<T...>;

    test_cq(void*) {}

    cn_t* connect() {
        cn_t* queue = new cn_t { "test-ipc-queue" };
        [&] { QVERIFY(queue->connect()); } ();
        return queue;
    }

    void disconnect(cn_t* queue) {
        queue->disconnect();
        delete queue;
    }

    void wait_start(int M) {
        cn_t que("test-ipc-queue");
        while (que.conn_count() != static_cast<std::size_t>(M)) {
            std::this_thread::yield();
        }
    }

    template <typename F>
    void recv(cn_t* queue, F&& proc) {
        while(1) {
            typename cn_t::value_t msg;
            while (!queue->pop(msg)) {
                std::this_thread::yield();
            }
            if (msg.pid_ < 0) return;
            proc(msg);
        }
    }

    cn_t* connect_send() {
        return new cn_t { "test-ipc-queue" };
    }

    void send(cn_t* cn, msg_t const & msg) {
        while (!cn->push(msg)) {
            std::this_thread::yield();
        }
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

constexpr int LoopCount = 1000000;
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

    std::cout << "sizeof(ea_t<sizeof(msg_t)>) = " << sizeof(*cq__) << std::endl;
}

template <int N, int M, bool V = true, int Loops = LoopCount>
void test_prod_cons() {
    benchmark_prod_cons<N, M, Loops, std::conditional_t<V, cq_t, void>>(cq__);
}

void Unit::test_prod_cons_1v1() {
//    ea_t<
//        sizeof(msg_t),
//        pc_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast>
//    > el_arr_mmb;
//    benchmark_prod_cons<1, 1, LoopCount, void>(&el_arr_mmb);
//    benchmark_prod_cons<2, 1, LoopCount, void>(&el_arr_mmb);

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast>
    > el_arr_ssu;
    benchmark_prod_cons<1, 1, LoopCount, cq_t>(&el_arr_ssu);
    benchmark_prod_cons<1, 1, LoopCount, void>(&el_arr_ssu);

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::single, ipc::relat::multi, ipc::trans::unicast>
    > el_arr_smu;
    benchmark_prod_cons<1, 1, LoopCount, decltype(el_arr_smu)::policy_t>(&el_arr_smu);
    benchmark_prod_cons<1, 1, LoopCount, void>(&el_arr_smu);

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::unicast>
    > el_arr_mmu;
    benchmark_prod_cons<1, 1, LoopCount, decltype(el_arr_mmu)::policy_t>(&el_arr_mmu);
    benchmark_prod_cons<1, 1, LoopCount, void>(&el_arr_mmu);

    test_prod_cons<1, 1>();
    test_prod_cons<1, 1, false>();

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast>
    > el_arr_mmb;
    benchmark_prod_cons<1, 1, LoopCount, cq_t>(&el_arr_mmb);
    benchmark_prod_cons<1, 1, LoopCount, void>(&el_arr_mmb);
}

void Unit::test_prod_cons_1v3() {
    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::single, ipc::relat::multi, ipc::trans::unicast>
    > el_arr_smu;
    benchmark_prod_cons<1, 3, LoopCount, decltype(el_arr_smu)::policy_t>(&el_arr_smu);
    benchmark_prod_cons<1, 3, LoopCount, void>(&el_arr_smu);

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::unicast>
    > el_arr_mmu;
    benchmark_prod_cons<1, 3, LoopCount, decltype(el_arr_mmu)::policy_t>(&el_arr_mmu);
    benchmark_prod_cons<1, 3, LoopCount, void>(&el_arr_mmu);

    test_prod_cons<1, 3>();
    test_prod_cons<1, 3, false>();

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast>
    > el_arr_mmb;
    benchmark_prod_cons<1, 3, LoopCount, cq_t>(&el_arr_mmb);
    benchmark_prod_cons<1, 3, LoopCount, void>(&el_arr_mmb);
}

void Unit::test_prod_cons_performance() {
    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::single, ipc::relat::multi, ipc::trans::unicast>
    > el_arr_smu;
    ipc::detail::static_for<8>([&el_arr_smu](auto index) {
        benchmark_prod_cons<1, decltype(index)::value + 1, LoopCount, void>(&el_arr_smu);
    });

    ipc::detail::static_for<8>([](auto index) {
        test_prod_cons<1, decltype(index)::value + 1, false>();
    });
    test_prod_cons<1, 8>(); // test & verify

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::unicast>
    > el_arr_mmu;
    ipc::detail::static_for<8>([&el_arr_mmu](auto index) {
        benchmark_prod_cons<1, decltype(index)::value + 1, LoopCount, void>(&el_arr_mmu);
    });
    ipc::detail::static_for<8>([&el_arr_mmu](auto index) {
        benchmark_prod_cons<decltype(index)::value + 1, 1, LoopCount, void>(&el_arr_mmu);
    });
    ipc::detail::static_for<8>([&el_arr_mmu](auto index) {
        benchmark_prod_cons<decltype(index)::value + 1, decltype(index)::value + 1, LoopCount, void>(&el_arr_mmu);
    });

    ea_t<
        sizeof(msg_t),
        pc_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast>
    > el_arr_mmb;
    ipc::detail::static_for<8>([&el_arr_mmb](auto index) {
        benchmark_prod_cons<1, decltype(index)::value + 1, LoopCount, void>(&el_arr_mmb);
    });
    ipc::detail::static_for<8>([&el_arr_mmb](auto index) {
        benchmark_prod_cons<decltype(index)::value + 1, 1, LoopCount, void>(&el_arr_mmb);
    });
    ipc::detail::static_for<8>([&el_arr_mmb](auto index) {
        benchmark_prod_cons<decltype(index)::value + 1, decltype(index)::value + 1, LoopCount, void>(&el_arr_mmb);
    });
}

void Unit::test_queue() {
    using queue_t = ipc::queue<msg_t, ipc::policy::choose<
            ipc::circ::elem_array,
            ipc::wr<ipc::relat::single, ipc::relat::multi, ipc::trans::broadcast>
    >>;
    queue_t queue;

    QVERIFY(!queue.push(msg_t { 1, 2 }));
    msg_t msg {};
    QVERIFY(!queue.pop(msg));
    QCOMPARE(msg, (msg_t {}));
    QVERIFY(sizeof(decltype(queue)::elems_t) <= sizeof(*cq__));

    ipc::detail::static_for<16>([](auto index) {
        benchmark_prod_cons<1, decltype(index)::value + 1, LoopCount>((queue_t*)nullptr);
    });
}

} // internal-linkage

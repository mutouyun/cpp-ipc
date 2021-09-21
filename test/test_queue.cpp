#include <iostream>
#include <string>
#include <type_traits>
#include <memory>
#include <new>
#include <vector>
#include <unordered_map>
#include <climits>  // CHAR_BIT

#include "libipc/prod_cons.h"
#include "libipc/policy.h"
#include "libipc/circ/elem_array.h"
#include "libipc/queue.h"

#include "test.h"

namespace {

struct msg_t {
    int pid_;
    int dat_;

    msg_t() = default;
    msg_t(int p, int d) : pid_(p), dat_(d) {}
};

template <ipc::relat Rp, ipc::relat Rc, ipc::trans Ts>
using queue_t = ipc::queue<msg_t, ipc::policy::choose<ipc::circ::elem_array, ipc::wr<Rp, Rc, Ts>>>;

template <ipc::relat Rp, ipc::relat Rc, ipc::trans Ts>
struct elems_t : public queue_t<Rp, Rc, Ts>::elems_t {};

bool operator==(msg_t const & m1, msg_t const & m2) noexcept {
    return (m1.pid_ == m2.pid_) && (m1.dat_ == m2.dat_);
}

bool operator!=(msg_t const & m1, msg_t const & m2) noexcept {
    return !(m1 == m2);
}

constexpr int LoopCount = 1000000;
constexpr int PushRetry = 1000000;
constexpr int ThreadMax = 8;

template <typename Que>
void push(Que & que, int p, int d) {
    for (int n = 0; !que.push([](void*) { return true; }, p, d); ++n) {
        ASSERT_NE(n, PushRetry);
        std::this_thread::yield();
    }
}

template <typename Que>
msg_t pop(Que & que) {
    msg_t msg;
    while (!que.pop(msg)) {
        std::this_thread::yield();
    }
    return msg;
}

template <ipc::trans Ts>
struct quitter;

template <>
struct quitter<ipc::trans::unicast> {
    template <typename Que>
    static void emit(Que && que, int r_cnt) {
        for (int k = 0; k < r_cnt; ++k) {
            push(que, -1, -1);
        }
    }
};

template <>
struct quitter<ipc::trans::broadcast> {
    template <typename Que>
    static void emit(Que && que, int /*r_cnt*/) {
        push(que, -1, -1);
    }
};

template <ipc::relat Rp, ipc::relat Rc, ipc::trans Ts>
void test_sr(elems_t<Rp, Rc, Ts> && elems, int s_cnt, int r_cnt, char const * message) {
    ipc_ut::sender().start(static_cast<std::size_t>(s_cnt));
    ipc_ut::reader().start(static_cast<std::size_t>(r_cnt));
    ipc_ut::test_stopwatch sw;

    for (int k = 0; k < s_cnt; ++k) {
        ipc_ut::sender() << [&elems, &sw, r_cnt, k] {
            queue_t<Rp, Rc, Ts> que { &elems };
            while (que.conn_count() != static_cast<std::size_t>(r_cnt)) {
                std::this_thread::yield();
            }
            sw.start();
            for (int i = 0; i < LoopCount; ++i) {
                push(que, k, i);
            }
        };
    }
    for (int k = 0; k < r_cnt; ++k) {
        ipc_ut::reader() << [&elems, k] {
            queue_t<Rp, Rc, Ts> que { &elems };
            ASSERT_TRUE(que.connect());
            while (pop(que).pid_ >= 0) ;
            ASSERT_TRUE(que.disconnect());
        };
    }

    ipc_ut::sender().wait_for_done();
    quitter<Ts>::emit(queue_t<Rp, Rc, Ts> { &elems }, r_cnt);
    ipc_ut::reader().wait_for_done();
    sw.print_elapsed(s_cnt, r_cnt, LoopCount, message);
}

} // internal-linkage

TEST(Queue, check_size) {
    using el_t = elems_t<ipc::relat::single, ipc::relat::multi, ipc::trans::broadcast>;

    std::cout << "cq_t::head_size  = " << el_t::head_size << std::endl;
    std::cout << "cq_t::data_size  = " << el_t::data_size << std::endl;
    std::cout << "cq_t::elem_size  = " << el_t::elem_size << std::endl;
    std::cout << "cq_t::block_size = " << el_t::block_size << std::endl;

    EXPECT_EQ(static_cast<std::size_t>(el_t::data_size), sizeof(msg_t));

    std::cout << "sizeof(elems_t<s, m, b>) = " << sizeof(el_t) << std::endl;
}

TEST(Queue, el_connection) {
    {
        elems_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> el;
        EXPECT_TRUE(el.connect_sender());
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_FALSE(el.connect_sender());
        }
        el.disconnect_sender();
        EXPECT_TRUE(el.connect_sender());
    }
    {
        elems_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::unicast> el;
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_TRUE(el.connect_sender());
        }
    }
    {
        elems_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> el;
        auto cc = el.connect_receiver();
        EXPECT_NE(cc, 0);
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_EQ(el.connect_receiver(), 0);
        }
        EXPECT_EQ(el.disconnect_receiver(cc), 0);
        EXPECT_EQ(el.connect_receiver(), cc);
    }
    {
        elems_t<ipc::relat::single, ipc::relat::multi, ipc::trans::broadcast> el;
        for (std::size_t i = 0; i < (sizeof(ipc::circ::cc_t) * CHAR_BIT); ++i) {
            ASSERT_NE(el.connect_receiver(), 0);
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_EQ(el.connect_receiver(), 0);
        }
    }
}

TEST(Queue, connection) {
    {
        elems_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> el;
        queue_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> que{&el};
        // sending
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_TRUE(que.ready_sending());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            queue_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> que{&el};
            ASSERT_FALSE(que.ready_sending());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            que.shut_sending();
        }
        {
            queue_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> que{&el};
            EXPECT_TRUE(que.ready_sending());
        }
        // receiving
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_TRUE(que.connect());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            queue_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> que{&el};
            ASSERT_FALSE(que.connect());
        }
        EXPECT_TRUE(que.disconnect());
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_FALSE(que.disconnect());
        }
        {
            queue_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> que{&el};
            EXPECT_TRUE(que.connect());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            queue_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast> que{&el};
            ASSERT_FALSE(que.connect());
        }
    }
    {
        elems_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> el;
        queue_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> que{&el};
        // sending
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_TRUE(que.ready_sending());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            queue_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> que{&el};
            ASSERT_TRUE(que.ready_sending());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            que.shut_sending();
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            queue_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> que{&el};
            ASSERT_TRUE(que.ready_sending());
        }
        // receiving
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_TRUE(que.connect());
        }
        for (std::size_t i = 1; i < (sizeof(ipc::circ::cc_t) * CHAR_BIT); ++i) {
            queue_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> que{&el};
            ASSERT_TRUE(que.connect());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            queue_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> que{&el};
            ASSERT_FALSE(que.connect());
        }
        ASSERT_TRUE(que.disconnect());
        for (std::size_t i = 0; i < 10000; ++i) {
            ASSERT_FALSE(que.disconnect());
        }
        {
            queue_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> que{&el};
            ASSERT_TRUE(que.connect());
        }
        for (std::size_t i = 0; i < 10000; ++i) {
            queue_t<ipc::relat::multi, ipc::relat::multi, ipc::trans::broadcast> que{&el};
            ASSERT_FALSE(que.connect());
        }
    }
}

TEST(Queue, prod_cons_1v1_unicast) {
    test_sr(elems_t<ipc::relat::single, ipc::relat::single, ipc::trans::unicast>{}, 1, 1, "ssu");
    test_sr(elems_t<ipc::relat::single, ipc::relat::multi , ipc::trans::unicast>{}, 1, 1, "smu");
    test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::unicast>{}, 1, 1, "mmu");
}

TEST(Queue, prod_cons_1v1_broadcast) {
    test_sr(elems_t<ipc::relat::single, ipc::relat::multi , ipc::trans::broadcast>{}, 1, 1, "smb");
    test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::broadcast>{}, 1, 1, "mmb");
}

TEST(Queue, prod_cons_1vN_unicast) {
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::single, ipc::relat::multi , ipc::trans::unicast>{}, 1, i, "smu");
    }
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::unicast>{}, 1, i, "mmu");
    }
}

TEST(Queue, prod_cons_1vN_broadcast) {
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::single, ipc::relat::multi , ipc::trans::broadcast>{}, 1, i, "smb");
    }
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::broadcast>{}, 1, i, "mmb");
    }
}

TEST(Queue, prod_cons_NvN_unicast) {
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::unicast>{}, 1, i, "mmu");
    }
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::unicast>{}, i, 1, "mmu");
    }
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::unicast>{}, i, i, "mmu");
    }
}

TEST(Queue, prod_cons_NvN_broadcast) {
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::broadcast>{}, 1, i, "mmb");
    }
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::broadcast>{}, i, 1, "mmb");
    }
    for (int i = 1; i <= ThreadMax; ++i) {
        test_sr(elems_t<ipc::relat::multi , ipc::relat::multi , ipc::trans::broadcast>{}, i, i, "mmb");
    }
}

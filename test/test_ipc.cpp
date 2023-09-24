
#include <vector>
#include <iostream>
#include <mutex>
#include <atomic>
#include <cstring>

#include "libipc/ipc.h"
#include "libipc/buffer.h"
#include "libipc/memory/resource.h"

#include "test.h"
#include "thread_pool.h"

#include "capo/random.hpp"

using namespace ipc;

namespace {

constexpr int LoopCount   = 10000;
constexpr int MultiMax    = 8;
constexpr int TestBuffMax = 65536;

struct msg_head {
    int id_;
};

class rand_buf : public buffer {
public:
    rand_buf() {
        int size = capo::random<>{(int)sizeof(msg_head), TestBuffMax}();
        *this = buffer(new char[size], size, [](void * p, std::size_t) {
            delete [] static_cast<char *>(p);
        });
    }

    rand_buf(msg_head const &msg) {
        *this = buffer(new msg_head{msg}, sizeof(msg), [](void * p, std::size_t) {
            delete static_cast<msg_head *>(p);
        });
    }

    rand_buf(rand_buf &&) = default;
    rand_buf(rand_buf const & rhs) {
        if (rhs.empty()) return;
        void * mem = new char[rhs.size()];
        std::memcpy(mem, rhs.data(), rhs.size());
        *this = buffer(mem, rhs.size(), [](void * p, std::size_t) {
            delete [] static_cast<char *>(p);
        });
    }

    rand_buf(buffer && rhs)
        : buffer(std::move(rhs)) {
    }

    void set_id(int k) noexcept {
        get<msg_head *>()->id_ = k;
    }

    int get_id() const noexcept {
        return get<msg_head *>()->id_;
    }

    using buffer::operator=;
};

template <relat Rp, relat Rc, trans Ts>
void test_basic(char const * name) {
    using que_t = chan<Rp, Rc, Ts>;
    rand_buf test1, test2;

    que_t que1 { name };
    EXPECT_FALSE(que1.send(test1));
    EXPECT_FALSE(que1.try_send(test2));

    que_t que2 { que1.name(), ipc::receiver };
    ASSERT_TRUE(que1.send(test1));
    ASSERT_TRUE(que1.try_send(test2));

    EXPECT_EQ(que2.recv(), test1);
    EXPECT_EQ(que2.recv(), test2);
}

class data_set {
    std::vector<rand_buf> datas_;

public:
    data_set() {
        datas_.resize(LoopCount);
        for (int i = 0; i < LoopCount; ++i) {
            datas_[i].set_id(i);
        }
    }

    std::vector<rand_buf> const &get() const noexcept {
        return datas_;
    }
} const data_set__;

template <relat Rp, relat Rc, trans Ts, typename Que = chan<Rp, Rc, Ts>>
void test_sr(char const * name, int s_cnt, int r_cnt) {
    ipc_ut::sender().start(static_cast<std::size_t>(s_cnt));
    ipc_ut::reader().start(static_cast<std::size_t>(r_cnt));

    std::atomic_thread_fence(std::memory_order_seq_cst);
    ipc_ut::test_stopwatch sw;

    for (int k = 0; k < s_cnt; ++k) {
        ipc_ut::sender() << [name, &sw, r_cnt, k] {
            Que que { name, ipc::sender };
            ASSERT_TRUE(que.wait_for_recv(r_cnt));
            sw.start();
            for (int i = 0; i < (int)data_set__.get().size(); ++i) {
                ASSERT_TRUE(que.send(data_set__.get()[i]));
            }
        };
    }

    for (int k = 0; k < r_cnt; ++k) {
        ipc_ut::reader() << [name] {
            Que que { name, ipc::receiver };
            for (;;) {
                rand_buf got { que.recv() };
                ASSERT_FALSE(got.empty());
                int i = got.get_id();
                if (i == -1) {
                    return;
                }
                ASSERT_TRUE((i >= 0) && (i < (int)data_set__.get().size()));
                auto const &data_set = data_set__.get()[i];
                if (data_set != got) {
                    printf("data_set__.get()[%d] != got, size = %zd/%zd\n", 
                            i, data_set.size(), got.size());
                    ASSERT_TRUE(false);
                }
            }
        };
    }

    ipc_ut::sender().wait_for_done();
    Que que { name };
    ASSERT_TRUE(que.wait_for_recv(r_cnt));
    for (int k = 0; k < r_cnt; ++k) {
        que.send(rand_buf{msg_head{-1}});
    }
    ipc_ut::reader().wait_for_done();
    sw.print_elapsed<std::chrono::microseconds>(s_cnt, r_cnt, (int)data_set__.get().size(), name);
}

} // internal-linkage

TEST(IPC, basic_ssu) {
    test_basic<relat::single, relat::single, trans::unicast  >("ssu");
}

TEST(IPC, basic_smb) {
    test_basic<relat::single, relat::multi , trans::broadcast>("smb");
}

TEST(IPC, basic_mmb) {
    test_basic<relat::multi , relat::multi , trans::broadcast>("mmb");
}

TEST(IPC, 1v1) {
    test_sr<relat::single, relat::single, trans::unicast  >("ssu", 1, 1);
    //test_sr<relat::single, relat::multi , trans::unicast  >("smu", 1, 1);
    //test_sr<relat::multi , relat::multi , trans::unicast  >("mmu", 1, 1);
    test_sr<relat::single, relat::multi , trans::broadcast>("smb", 1, 1);
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", 1, 1);
}

TEST(IPC, 1vN) {
    //test_sr<relat::single, relat::multi , trans::unicast  >("smu", 1, MultiMax);
    //test_sr<relat::multi , relat::multi , trans::unicast  >("mmu", 1, MultiMax);
    test_sr<relat::single, relat::multi , trans::broadcast>("smb", 1, MultiMax);
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", 1, MultiMax);
}

TEST(IPC, Nv1) {
    //test_sr<relat::multi , relat::multi , trans::unicast  >("mmu", MultiMax, 1);
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", MultiMax, 1);
}

TEST(IPC, NvN) {
    //test_sr<relat::multi , relat::multi , trans::unicast  >("mmu", MultiMax, MultiMax);
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", MultiMax, MultiMax);
}

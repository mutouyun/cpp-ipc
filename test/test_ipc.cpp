
#include <vector>
#include <iostream>
#include <cstring>

#include "libipc/ipc.h"
#include "libipc/buffer.h"
#include "libipc/memory/resource.h"

#include "test.h"
#include "thread_pool.h"

#include "capo/random.hpp"

using namespace ipc;

namespace {

class rand_buf : public buffer {
public:
    rand_buf() {
        int size = capo::random<>{1, 65536}();
        *this = buffer(new char[size], size, [](void * p, std::size_t) {
            delete [] static_cast<char *>(p);
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
        *get<char *>() = static_cast<char>(k);
    }

    int get_id() const noexcept {
        return static_cast<int>(*get<char *>());
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
    EXPECT_TRUE(que1.send(test1));
    EXPECT_TRUE(que1.try_send(test2));

    EXPECT_EQ(que2.recv(), test1);
    EXPECT_EQ(que2.recv(), test2);
}

template <relat Rp, relat Rc, trans Ts>
void test_sr(char const * name, int size, int s_cnt, int r_cnt) {
    using que_t = chan<Rp, Rc, Ts>;

    ipc_ut::sender().start(static_cast<std::size_t>(s_cnt));
    ipc_ut::reader().start(static_cast<std::size_t>(r_cnt));
    ipc_ut::test_stopwatch sw;
    std::vector<rand_buf> tests(size);

    for (int k = 0; k < s_cnt; ++k) {
        ipc_ut::sender() << [name, &tests, &sw, r_cnt, k] {
            que_t que1 { name };
            EXPECT_TRUE(que1.wait_for_recv(r_cnt));
            sw.start();
            for (auto & buf : tests) {
                rand_buf data { buf };
                data.set_id(k);
                EXPECT_TRUE(que1.send(data));
            }
        };
    }

    for (int k = 0; k < r_cnt; ++k) {
        ipc_ut::reader() << [name, &tests, s_cnt] {
            que_t que2 { name, ipc::receiver };
            std::vector<int> cursors(s_cnt);
            for (;;) {
                rand_buf got { que2.recv() };
                ASSERT_FALSE(got.empty());
                int & cur = cursors.at(got.get_id());
                ASSERT_TRUE((cur >= 0) && (cur < static_cast<int>(tests.size())));
                rand_buf buf { tests.at(cur++) };
                buf.set_id(got.get_id());
                EXPECT_EQ(got, buf);
                int n = 0;
                for (; n < static_cast<int>(cursors.size()); ++n) {
                    if (cursors[n] < static_cast<int>(tests.size())) break;
                }
                if (n == static_cast<int>(cursors.size())) break;
            }
        };
    }

    ipc_ut::sender().wait_for_done();
    ipc_ut::reader().wait_for_done();
    sw.print_elapsed<std::chrono::microseconds>(s_cnt, r_cnt, size, name);
}

constexpr int LoopCount = 10000;
constexpr int MultiMax  = 8;

} // internal-linkage

TEST(IPC, basic) {
    test_basic<relat::single, relat::single, trans::unicast  >("ssu");
    test_basic<relat::single, relat::multi , trans::unicast  >("smu");
    test_basic<relat::multi , relat::multi , trans::unicast  >("mmu");
    test_basic<relat::single, relat::multi , trans::broadcast>("smb");
    test_basic<relat::multi , relat::multi , trans::broadcast>("mmb");
}

TEST(IPC, 1v1) {
    test_sr<relat::single, relat::single, trans::unicast  >("ssu", LoopCount, 1, 1);
    test_sr<relat::single, relat::multi , trans::unicast  >("smu", LoopCount, 1, 1);
    test_sr<relat::multi , relat::multi , trans::unicast  >("mmu", LoopCount, 1, 1);
    test_sr<relat::single, relat::multi , trans::broadcast>("smb", LoopCount, 1, 1);
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", LoopCount, 1, 1);
}

TEST(IPC, 1vN) {
    test_sr<relat::single, relat::multi , trans::broadcast>("smb", LoopCount, 1, MultiMax);
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", LoopCount, 1, MultiMax);
}

TEST(IPC, Nv1) {
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", LoopCount, MultiMax, 1);
}

TEST(IPC, NvN) {
    test_sr<relat::multi , relat::multi , trans::broadcast>("mmb", LoopCount, MultiMax, MultiMax);
}

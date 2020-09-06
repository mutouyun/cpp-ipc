#include <cstring>
#include <cstdint>
#include <thread>

#include "shm.h"
#include "test.h"

using namespace ipc::shm;

namespace {

handle shm_hd__;

TEST(SHM, acquire) {
    EXPECT_TRUE(!shm_hd__.valid());

    EXPECT_TRUE(shm_hd__.acquire("my-test-1", 1024));
    EXPECT_TRUE(shm_hd__.valid());
    EXPECT_STREQ(shm_hd__.name(), "my-test-1");

    EXPECT_TRUE(shm_hd__.acquire("my-test-2", 2048));
    EXPECT_TRUE(shm_hd__.valid());
    EXPECT_STREQ(shm_hd__.name(), "my-test-2");

    EXPECT_TRUE(shm_hd__.acquire("my-test-3", 4096));
    EXPECT_TRUE(shm_hd__.valid());
    EXPECT_STREQ(shm_hd__.name(), "my-test-3");
}

TEST(SHM, release) {
    EXPECT_TRUE(shm_hd__.valid());
    shm_hd__.release();
    EXPECT_TRUE(!shm_hd__.valid());
}

TEST(SHM, get) {
    EXPECT_TRUE(shm_hd__.get() == nullptr);
    EXPECT_TRUE(shm_hd__.acquire("my-test", 1024));

    auto mem = shm_hd__.get();
    EXPECT_TRUE(mem != nullptr);
    EXPECT_TRUE(mem == shm_hd__.get());

    std::uint8_t buf[1024] = {};
    EXPECT_TRUE(memcmp(mem, buf, sizeof(buf)) == 0);

    handle shm_other(shm_hd__.name(), shm_hd__.size());
    EXPECT_TRUE(shm_other.get() != shm_hd__.get());
}

TEST(SHM, hello) {
    auto mem = shm_hd__.get();
    EXPECT_TRUE(mem != nullptr);

    constexpr char hello[] = "hello!";
    std::memcpy(mem, hello, sizeof(hello));
    EXPECT_STREQ((char const *)shm_hd__.get(), hello);

    shm_hd__.release();
    EXPECT_TRUE(shm_hd__.get() == nullptr);
    EXPECT_TRUE(shm_hd__.acquire("my-test", 1024));

    mem = shm_hd__.get();
    EXPECT_TRUE(mem != nullptr);
    std::uint8_t buf[1024] = {};
    EXPECT_TRUE(memcmp(mem, buf, sizeof(buf)) == 0);

    std::memcpy(mem, hello, sizeof(hello));
    EXPECT_STREQ((char const *)shm_hd__.get(), hello);
}

TEST(SHM, mt) {
    std::thread {
        [] {
            handle shm_mt(shm_hd__.name(), shm_hd__.size());

            shm_hd__.release();

            constexpr char hello[] = "hello!";
            EXPECT_STREQ((char const *)shm_mt.get(), hello);
        }
    }.join();
    EXPECT_TRUE(shm_hd__.get() == nullptr);
    EXPECT_TRUE(!shm_hd__.valid());

    EXPECT_TRUE(shm_hd__.acquire("my-test", 1024));
    std::uint8_t buf[1024] = {};
    EXPECT_TRUE(memcmp(shm_hd__.get(), buf, sizeof(buf)) == 0);
}

} // internal-linkage

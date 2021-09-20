#include <thread>
#include <iostream>

#include "libipc/waiter.h"
#include "test.h"

namespace {

TEST(Waiter, broadcast) {
    ipc::detail::waiter waiter;
    std::thread ts[10];

    int k = 0;
    for (auto& t : ts) {
        t = std::thread([&k] {
            ipc::detail::waiter waiter {"test-ipc-waiter"};
            EXPECT_TRUE(waiter.valid());
            for (int i = 0; i < 99; ++i) {
                ASSERT_TRUE(waiter.wait_if([&k, &i] { return k == i; }));
            }
        });
    }

    EXPECT_TRUE(waiter.open("test-ipc-waiter"));
    std::cout << "waiting for broadcast...\n";
    for (k = 1; k < 100; ++k) {
        std::cout << "broadcast: " << k << "\n";
        ASSERT_TRUE(waiter.broadcast());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for (auto& t : ts) t.join();
}

} // internal-linkage

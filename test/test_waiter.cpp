#include <thread>
#include <iostream>

#include "libipc/platform/waiter_wrapper.h"
#include "test.h"

namespace {

TEST(Waiter, broadcast) {
    ipc::detail::waiter w;
    std::thread ts[10];

    for (auto& t : ts) {
        t = std::thread([&w] {
            ipc::detail::waiter_wrapper wp { &w };
            EXPECT_TRUE(wp.open("test-ipc-waiter"));
            EXPECT_TRUE(wp.wait_if([] { return true; }));
            wp.close();
        });
    }

    ipc::detail::waiter_wrapper wp { &w };
    EXPECT_TRUE(wp.open("test-ipc-waiter"));

    std::cout << "waiting for broadcast...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(wp.broadcast());

    for (auto& t : ts) t.join();
    wp.close();
}

} // internal-linkage

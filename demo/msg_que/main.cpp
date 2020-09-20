
#include <signal.h>

#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstddef>

#include "libipc/ipc.h"
#include "capo/random.hpp"

namespace {

constexpr char const name__  [] = "ipc-msg-que";
constexpr char const mode_s__[] = "s";
constexpr char const mode_r__[] = "r";

constexpr std::size_t const min_sz = 128;
constexpr std::size_t const max_sz = 1024 * 16;

std::atomic<bool> is_quit__{ false };
std::atomic<std::size_t> size_per_1s__{ 0 };

using msg_que_t = ipc::chan<ipc::relat::single, ipc::relat::single, ipc::trans::unicast>;

msg_que_t que__{ name__ };
ipc::byte_t buff__[max_sz];
capo::random<> rand__{ min_sz, max_sz };

inline std::string str_of_size(std::size_t sz) noexcept {
    if (sz <= 1024) {
        return std::to_string(sz) + " bytes";
    }
    if (sz <= 1024 * 1024) {
        return std::to_string(sz / 1024) + " KB";
    }
    return std::to_string(sz / (1024 * 1024)) + " MB";
}

inline std::string speed_of(std::size_t sz) noexcept {
    return str_of_size(sz) + "/s";
}

void do_counting() {
    for (int i = 1; !is_quit__.load(std::memory_order_acquire); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100 ms
        if (i % 10) continue;
        i = 0;
        std::cout
            << speed_of(size_per_1s__.load(std::memory_order_acquire))
            << std::endl;
        size_per_1s__.store(0, std::memory_order_release);
    }
}

void do_send() {
    std::cout 
        << __func__ << ": start [" 
        << str_of_size(min_sz) << " - " << str_of_size(max_sz) 
        << "]...\n";
    if (!que__.reconnect(ipc::sender)) {
        std::cerr << __func__ << ": connect failed.\n";
    }
    else {
        std::thread counting{ do_counting };
        while (!is_quit__.load(std::memory_order_acquire)) {
            std::size_t sz = static_cast<std::size_t>(rand__());
            if (!que__.send(ipc::buff_t(buff__, sz))) {
                std::cerr << __func__ << ": send failed.\n";
                std::cout << __func__ << ": waiting for receiver...\n";
                if (!que__.wait_for_recv(1)) {
                    std::cerr << __func__ << ": wait receiver failed.\n";
                    is_quit__.store(true, std::memory_order_release);
                    break;
                }
            }
            size_per_1s__.fetch_add(sz, std::memory_order_release);
            std::this_thread::yield();
        }
        counting.join();
    }
    std::cout << __func__ << ": quit...\n";
}

void do_recv() {
    std::cout
        << __func__ << ": start ["
        << str_of_size(min_sz) << " - " << str_of_size(max_sz)
        << "]...\n";
    if (!que__.reconnect(ipc::receiver)) {
        std::cerr << __func__ << ": connect failed.\n";
    }
    else {
        std::thread counting{ do_counting };
        while (!is_quit__.load(std::memory_order_acquire)) {
            auto msg = que__.recv();
            if (msg.empty()) break;
            size_per_1s__.fetch_add(msg.size(), std::memory_order_release);
        }
        counting.join();
    }
    std::cout << __func__ << ": quit...\n";
}

} // namespace

int main(int argc, char ** argv) {
    if (argc < 2) return 0;

    ::signal(SIGINT, [](int) {
        is_quit__.store(true, std::memory_order_release);
        que__.disconnect();
    });

    if (std::string{ argv[1] } == mode_s__) {
        do_send();
    }
    else if (std::string{ argv[1] } == mode_r__) {
        do_recv();
    }
    return 0;
}

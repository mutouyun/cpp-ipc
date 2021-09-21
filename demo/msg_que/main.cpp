
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

std::atomic<bool> is_quit__ {false};
std::atomic<std::size_t> size_counter__ {0};

using msg_que_t = ipc::chan<ipc::relat::single, ipc::relat::multi, ipc::trans::broadcast>;

msg_que_t que__{ name__ };
ipc::byte_t buff__[max_sz];
capo::random<> rand__{ 
    static_cast<int>(min_sz), 
    static_cast<int>(max_sz)
};

inline std::string str_of_size(std::size_t sz) noexcept {
    if (sz > 1024 * 1024) {
        return std::to_string(sz / (1024 * 1024)) + " MB";
    }
    if (sz > 1024) {
        return std::to_string(sz / 1024) + " KB";
    }
    return std::to_string(sz) + " bytes";
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
            << speed_of(size_counter__.exchange(0, std::memory_order_relaxed))
            << std::endl;
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
            size_counter__.fetch_add(sz, std::memory_order_relaxed);
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
            size_counter__.fetch_add(msg.size(), std::memory_order_relaxed);
        }
        counting.join();
    }
    std::cout << __func__ << ": quit...\n";
}

} // namespace

int main(int argc, char ** argv) {
    if (argc < 2) return 0;

    auto exit = [](int) {
        is_quit__.store(true, std::memory_order_release);
        que__.disconnect();
    };
    ::signal(SIGINT  , exit);
    ::signal(SIGABRT , exit);
    ::signal(SIGSEGV , exit);
    ::signal(SIGTERM , exit);
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)
    ::signal(SIGBREAK, exit);
#else
    ::signal(SIGHUP  , exit);
#endif

    std::string mode {argv[1]};
    if (mode == mode_s__) {
        do_send();
    } else if (mode == mode_r__) {
        do_recv();
    }
    return 0;
}

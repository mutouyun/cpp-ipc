
#include <signal.h>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

#include "libipc/ipc.h"

namespace {

std::atomic<bool> is_quit__ {false};
ipc::channel *ipc__ = nullptr;

void do_send(int size, int interval) {
    ipc::channel ipc {"ipc", ipc::sender};
    ipc__ = &ipc;
    std::string buffer(size, 'A');
    while (!is_quit__.load(std::memory_order_acquire)) {
        std::cout << "send size: " << buffer.size() + 1 << "\n";
        ipc.send(buffer, 0/*tm*/);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}

void do_recv(int interval) {
    ipc::channel ipc {"ipc", ipc::receiver};
    ipc__ = &ipc;
    while (!is_quit__.load(std::memory_order_acquire)) {
        ipc::buff_t recv;
        for (int k = 1; recv.empty(); ++k) {
            std::cout << "recv waiting... " << k << "\n";
            recv = ipc.recv(interval);
            if (is_quit__.load(std::memory_order_acquire)) return;
        }
        std::cout << "recv size: " << recv.size() << "\n";
    }
}

} // namespace

int main(int argc, char ** argv) {
    if (argc < 3) return -1;

    auto exit = [](int) {
        is_quit__.store(true, std::memory_order_release);
        if (ipc__ != nullptr) ipc__->disconnect();
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
    if (mode == "send") {
        if (argc < 4) return -1;
        do_send(std::stoi(argv[2]) /*size*/, 
                std::stoi(argv[3]) /*interval*/);
    } else if (mode == "recv") {
        do_recv(std::stoi(argv[2]) /*interval*/);
    }
    return 0;
}

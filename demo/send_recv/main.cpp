
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "libipc/ipc.h"

namespace {

void do_send(int size, int interval) {
    ipc::channel ipc {"ipc", ipc::sender};
    std::string buffer(size, 'A');
    while (true) {
        std::cout << "send size: " << buffer.size() + 1 << "\n";
        ipc.send(buffer, 0/*tm*/);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}

void do_recv(int interval) {
    ipc::channel ipc {"ipc", ipc::receiver};
    while (true) {
        ipc::buff_t recv;
        for (int k = 1; recv.empty(); ++k) {
            std::cout << "recv waiting... " << k << "\n";
            recv = ipc.recv(interval);
        }
        std::cout << "recv size: " << recv.size() << "\n";
    }
}

} // namespace

int main(int argc, char ** argv) {
    if (argc < 3) return -1;
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

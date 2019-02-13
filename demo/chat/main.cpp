#include <iostream>
#include <string>
#include <thread>

#include "ipc.h"

namespace {

char name__[] = "ipc-chat";
char quit__[] = "q";

} // namespace

int main() {
    std::string buf;
    ipc::channel cc { name__ };

    std::thread r {[] {
        ipc::channel cc { name__ };
        while (1) {
            auto buf = cc.recv();
            if (buf.empty()) continue;
            std::string dat { static_cast<char const *>(buf.data()), buf.size() - 1 };
            if (dat == quit__) {
                std::cout << "receiver quit..." << std::endl;
                return;
            }
            std::cout << "> " << dat << std::endl;
        }
    }};

    while (1) {
        std::cin >> buf;
        cc.send(buf);
        if (buf == quit__) break;
    }

    r.join();
    return 0;
}

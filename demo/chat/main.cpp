#include <iostream>
#include <string>
#include <thread>
#include <regex>
#include <atomic>

#include "ipc.h"

namespace {

char name__[] = "ipc-chat";
char quit__[] = "q";
char id__  [] = "c";

std::size_t calc_unique_id() {
    static ipc::shm::handle g_shm { "__CHAT_ACC_STORAGE__", sizeof(std::atomic<std::size_t>) };
    return static_cast<std::atomic<std::size_t>*>(g_shm.get())->fetch_add(1, std::memory_order_relaxed);
}

} // namespace

int main() {
    std::string buf, id = id__ + std::to_string(calc_unique_id());
    std::regex  reg { "(c\\d+)> (.*)" };

    ipc::channel cc { name__ };

    std::thread r {[&id, &reg] {
        ipc::channel cc { name__ };
        std::cout << id << " is ready." << std::endl;
        while (1) {
            auto buf = cc.recv();
            if (buf.empty()) continue;
            std::string dat { buf.get<char const *>(), buf.size() - 1 };
            std::smatch mid;
            if (std::regex_match(dat, mid, reg)) {
                if (mid.str(1) == id) {
                    if (mid.str(2) == quit__) {
                        std::cout << "receiver quit..." << std::endl;
                        return;
                    }
                    continue;
                }
            }
            std::cout << dat << std::endl;
        }
    }};

    while (1) {
        std::cin >> buf;
        cc.send(id + "> " + buf);
        if (buf == quit__) break;
    }

    r.join();
    return 0;
}

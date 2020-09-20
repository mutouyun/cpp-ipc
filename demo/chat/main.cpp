
#include <iostream>
#include <string>
#include <thread>
#include <regex>
#include <atomic>

#include "libipc/ipc.h"

namespace {

constexpr char const name__[] = "ipc-chat";
constexpr char const quit__[] = "q";
constexpr char const id__  [] = "c";

inline std::size_t calc_unique_id() {
    static ipc::shm::handle g_shm { "__CHAT_ACC_STORAGE__", sizeof(std::atomic<std::size_t>) };
    return static_cast<std::atomic<std::size_t>*>(g_shm.get())->fetch_add(1, std::memory_order_relaxed);
}

ipc::channel sender__   { name__, ipc::sender   };
ipc::channel receiver__ { name__, ipc::receiver };

} // namespace

int main() {
    std::string buf, id = id__ + std::to_string(calc_unique_id());
    std::regex  reg { "(c\\d+)> (.*)" };

    std::thread r {[&id, &reg] {
        std::cout << id << " is ready." << std::endl;
        while (1) {
            ipc::buff_t buf = receiver__.recv();
            if (buf.empty()) break; // quit
            std::string dat { buf.get<char const *>(), buf.size() - 1 };
            std::smatch mid;
            if (std::regex_match(dat, mid, reg)) {
                if (mid.str(1) == id) {
                    if (mid.str(2) == quit__) {
                        break;
                    }
                    continue;
                }
            }
            std::cout << dat << std::endl;
        }
        std::cout << id << " receiver is quit..." << std::endl;
    }};

    for (/*int i = 1*/;; /*++i*/) {
        std::cin >> buf;
        if (buf.empty() || (buf == quit__)) break;
//        std::cout << "[" << i << "]" << std::endl;
        sender__.send(id + "> " + buf);
        buf.clear();
    }

    receiver__.disconnect();
    r.join();
    std::cout << id << " sender is quit..." << std::endl;
    return 0;
}

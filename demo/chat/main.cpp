#include <iostream>
#include <string>
#include <thread>
#include <regex>

#include "ipc.h"

namespace ipc {
namespace detail {

IPC_EXPORT std::size_t calc_unique_id();

} // namespace detail
} // namespace ipc

namespace {

char name__[] = "ipc-chat";
char quit__[] = "q";
char id__  [] = "c";

} // namespace

int main() {
    std::string buf, id = id__ + std::to_string(ipc::detail::calc_unique_id());
    std::regex  reg { "(c\\d+)> (.*)" };

    ipc::channel cc { name__ };

    std::thread r {[&id, &reg] {
        ipc::channel cc { name__ };
        std::cout << id << " is ready." << std::endl;
        while (1) {
            auto buf = cc.recv();
            if (buf.empty()) continue;
            std::string dat { static_cast<char const *>(buf.data()), buf.size() - 1 };
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

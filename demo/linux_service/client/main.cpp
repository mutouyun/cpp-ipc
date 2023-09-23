/// \brief To create a basic command line program.

#include <stdio.h>
#include <thread>
#include <chrono>

#include "libipc/ipc.h"

int main(int argc, char *argv[]) {
    printf("My Sample Client: Entry\n");
    ipc::channel ipc_r{"service ipc r", ipc::receiver};
    ipc::channel ipc_w{"service ipc w", ipc::sender};
    while (1) {
        auto msg = ipc_r.recv();
        if (msg.empty()) {
            printf("My Sample Client: message recv error\n");
            return -1;
        }
        printf("My Sample Client: message recv: [%s]\n", (char const *)msg.data());
        while (!ipc_w.send("Copy.")) {
            printf("My Sample Client: message send error\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        printf("My Sample Client: message send [Copy]\n");
    }
    printf("My Sample Client: Exit\n");
    return 0;
}

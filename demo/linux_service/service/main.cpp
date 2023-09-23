/// \brief To create a basic command line program.

#include <stdio.h>
#include <string>
#include <thread>
#include <chrono>

#include "libipc/ipc.h"

int main(int argc, char *argv[]) {
    printf("My Sample Service: Main: Entry\n");

    ipc::channel ipc_r{"service ipc r", ipc::sender};
    ipc::channel ipc_w{"service ipc w", ipc::receiver};

    while (1) {
        if (!ipc_r.send("Hello, World!")) {
            printf("My Sample Service: send failed.\n");
        }
        else {
            printf("My Sample Service: send [Hello, World!]\n");
            auto msg = ipc_w.recv(1000);
            if (msg.empty()) {
                printf("My Sample Service: recv error\n");
            } else {
                printf("%s\n", (std::string{"My Sample Service: recv ["} + msg.get<char const *>() + "]").c_str());
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    printf("My Sample Service: Main: Exit\n");
    return 0;
}

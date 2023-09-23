/// \brief To create a basic Windows command line program.

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#include "libipc/ipc.h"

int _tmain (int argc, TCHAR *argv[]) {
    _tprintf(_T("My Sample Client: Entry\n"));
    ipc::channel ipc_r{"service ipc r", ipc::receiver};
    ipc::channel ipc_w{"service ipc w", ipc::sender};
    while (1) {
        auto msg = ipc_r.recv();
        if (msg.empty()) {
            _tprintf(_T("My Sample Client: message recv error\n"));
            return -1;
        }
        printf("My Sample Client: message recv: [%s]\n", (char const *)msg.data());
        while (!ipc_w.send("Copy.")) {
            _tprintf(_T("My Sample Client: message send error\n"));
            Sleep(1000);
        }
        _tprintf(_T("My Sample Client: message send [Copy]\n"));
    }
    _tprintf(_T("My Sample Client: Exit\n"));
    return 0;
}

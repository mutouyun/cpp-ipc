/// \brief To create a basic Windows command line program.

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#include "libipc/ipc.h"

int _tmain (int argc, TCHAR *argv[]) {
    _tprintf(_T("My Sample Client: Entry\n"));
    ipc::channel ipc_r{ipc::prefix{"Global\\"}, "service ipc r", ipc::receiver};
    ipc::channel ipc_w{ipc::prefix{"Global\\"}, "service ipc w", ipc::sender};
    while (1) {
        if (!ipc_r.reconnect(ipc::receiver)) {
            Sleep(1000);
            continue;
        }
        auto msg = ipc_r.recv();
        if (msg.empty()) {
            _tprintf(_T("My Sample Client: message recv error\n"));
            ipc_r.disconnect();
            continue;
        }
        printf("My Sample Client: message recv: [%s]\n", (char const *)msg.data());
        for (;;) {
            if (!ipc_w.reconnect(ipc::sender)) {
                Sleep(1000);
                continue;
            }
            if (ipc_w.send("Copy.")) {
                break;
            }
            _tprintf(_T("My Sample Client: message send error\n"));
            ipc_w.disconnect();
            Sleep(1000);
        }
        _tprintf(_T("My Sample Client: message send [Copy]\n"));
    }
    _tprintf(_T("My Sample Client: Exit\n"));
    return 0;
}

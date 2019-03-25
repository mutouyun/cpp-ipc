#include "shm.h"

#include <Windows.h>

#include <string>
#include <utility>

#include "def.h"
#include "log.h"
#include "memory/resource.h"
#include "platform/to_tchar.h"

namespace ipc {
namespace shm {

id_t acquire(char const * name, std::size_t size) {
    if (name == nullptr || name[0] == '\0' || size == 0) {
        return nullptr;
    }
    HANDLE h = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
                                   PAGE_READWRITE | SEC_COMMIT,
                                   0, static_cast<DWORD>(size),
                                   ipc::detail::to_tchar(std::string{"__IPC_SHM__"} + name).c_str());
    if (h == NULL) {
        ipc::error("fail CreateFileMapping[%d]: %s\n", static_cast<int>(::GetLastError()), name);
        return nullptr;
    }
    return static_cast<id_t>(h);
}

void * to_mem(id_t id) {
    if (id == nullptr) return nullptr;
    HANDLE h = static_cast<HANDLE>(id);
    LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (mem == NULL) {
        ipc::error("fail MapViewOfFile[%d]\n", static_cast<int>(::GetLastError()));
        return nullptr;
    }
    return static_cast<void *>(mem);
}

void release(id_t id, void * mem, std::size_t /*size*/) {
    if (id  == nullptr) return;
    if (mem == nullptr) return;
    ::UnmapViewOfFile(static_cast<LPVOID>(mem));
    ::CloseHandle(static_cast<HANDLE>(id));
}

} // namespace shm
} // namespace ipc

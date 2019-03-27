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

id_t acquire(char const * name, std::size_t size, unsigned mode) {
    if (name == nullptr || name[0] == '\0' || size == 0) {
        return nullptr;
    }
    HANDLE h;
    auto fmt_name = ipc::detail::to_tchar(std::string{"__IPC_SHM__"} + name);
    // Opens a named file mapping object.
    if (mode == open) {
        h = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, fmt_name.c_str());
    }
    // Creates or opens a named file mapping object for a specified file.
    else {
        h = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT,
                                0, static_cast<DWORD>(size), fmt_name.c_str());
        // If the object exists before the function call, the function returns a handle to the existing object 
        // (with its current size, not the specified size), and GetLastError returns ERROR_ALREADY_EXISTS.
        if ((mode == create) && (::GetLastError() == ERROR_ALREADY_EXISTS)) {
            ::CloseHandle(h);
            h = NULL;
        }
    }
    if (h == NULL) {
        ipc::error("fail CreateFileMapping/OpenFileMapping[%d]: %s\n", static_cast<int>(::GetLastError()), name);
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

void remove(char const * /*name*/) {
}

} // namespace shm
} // namespace ipc

#include "shm.h"

#include <Windows.h>

#include <string>
#include <utility>
#include <mutex>

#include "def.h"
#include "log.h"
#include "memory/resource.h"
#include "platform/to_tchar.h"

namespace {

inline auto* m2h() {
    static struct {
        std::mutex lc_;
        ipc::mem::unordered_map<void*, HANDLE> cache_;
    } m2h_;
    return &m2h_;
}

} // internal-linkage

namespace ipc {
namespace shm {

void* acquire(char const * name, std::size_t size) {
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
    LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (mem == NULL) {
        ipc::error("fail MapViewOfFile[%d]: %s\n", static_cast<int>(::GetLastError()), name);
        ::CloseHandle(h);
        return nullptr;
    }
    {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(m2h()->lc_);
        m2h()->cache_.emplace(mem, h);
    }
    return mem;
}

void release(void* mem, std::size_t /*size*/) {
    if (mem == nullptr) {
        return;
    }
    IPC_UNUSED_ auto guard = ipc::detail::unique_lock(m2h()->lc_);
    auto& cc = m2h()->cache_;
    auto it = cc.find(mem);
    if (it == cc.end()) {
        return;
    }
    ::UnmapViewOfFile(mem);
    ::CloseHandle(it->second);
    cc.erase(it);
}

} // namespace shm
} // namespace ipc

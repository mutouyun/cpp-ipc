
#include <Windows.h>

#include <string>
#include <utility>

#include "libipc/shm.h"
#include "libipc/def.h"
#include "libipc/pool_alloc.h"

#include "libipc/utility/log.h"
#include "libipc/memory/resource.h"

#include "to_tchar.h"
#include "get_sa.h"

namespace {

struct id_info_t {
    HANDLE      h_    = NULL;
    void*       mem_  = nullptr;
    std::size_t size_ = 0;
};

} // internal-linkage

namespace ipc {
namespace shm {

id_t acquire(char const * name, std::size_t size, unsigned mode) {
    if (!is_valid_string(name)) {
        ipc::error("fail acquire: name is empty\n");
        return nullptr;
    }
    HANDLE h;
    auto fmt_name = ipc::detail::to_tchar(name);
    // Opens a named file mapping object.
    if (mode == open) {
        h = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, fmt_name.c_str());
        if (h == NULL) {
          ipc::error("fail OpenFileMapping[%d]: %s\n", static_cast<int>(::GetLastError()), name);
          return nullptr;
        }
    }
    // Creates or opens a named file mapping object for a specified file.
    else {
        h = ::CreateFileMapping(INVALID_HANDLE_VALUE, detail::get_sa(), PAGE_READWRITE | SEC_COMMIT,
                                0, static_cast<DWORD>(size), fmt_name.c_str());
        DWORD err = ::GetLastError();
        // If the object exists before the function call, the function returns a handle to the existing object 
        // (with its current size, not the specified size), and GetLastError returns ERROR_ALREADY_EXISTS.
        if ((mode == create) && (err == ERROR_ALREADY_EXISTS)) {
            if (h != NULL) ::CloseHandle(h);
            h = NULL;
        }
        if (h == NULL) {
          ipc::error("fail CreateFileMapping[%d]: %s\n", static_cast<int>(err), name);
          return nullptr;
        }
    }
    auto ii = mem::alloc<id_info_t>();
    ii->h_    = h;
    ii->size_ = size;
    return ii;
}

std::int32_t get_ref(id_t) {
    return 0;
}

void sub_ref(id_t) {
    // Do Nothing.
}

void * get_mem(id_t id, std::size_t * size) {
    if (id == nullptr) {
        ipc::error("fail get_mem: invalid id (null)\n");
        return nullptr;
    }
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ != nullptr) {
        if (size != nullptr) *size = ii->size_;
        return ii->mem_;
    }
    if (ii->h_ == NULL) {
        ipc::error("fail to_mem: invalid id (h = null)\n");
        return nullptr;
    }
    LPVOID mem = ::MapViewOfFile(ii->h_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (mem == NULL) {
        ipc::error("fail MapViewOfFile[%d]\n", static_cast<int>(::GetLastError()));
        return nullptr;
    }
    MEMORY_BASIC_INFORMATION mem_info;
    if (::VirtualQuery(mem, &mem_info, sizeof(mem_info)) == 0) {
        ipc::error("fail VirtualQuery[%d]\n", static_cast<int>(::GetLastError()));
        return nullptr;
    }
    ii->mem_  = mem;
    ii->size_ = static_cast<std::size_t>(mem_info.RegionSize);
    if (size != nullptr) *size = ii->size_;
    return static_cast<void *>(mem);
}

std::int32_t release(id_t id) {
    if (id == nullptr) {
        ipc::error("fail release: invalid id (null)\n");
        return -1;
    }
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ == nullptr || ii->size_ == 0) {
        ipc::error("fail release: invalid id (mem = %p, size = %zd)\n", ii->mem_, ii->size_);
    }
    else ::UnmapViewOfFile(static_cast<LPCVOID>(ii->mem_));
    if (ii->h_ == NULL) {
        ipc::error("fail release: invalid id (h = null)\n");
    }
    else ::CloseHandle(ii->h_);
    mem::free(ii);
    return 0;
}

void remove(id_t id) {
    if (id == nullptr) {
        ipc::error("fail release: invalid id (null)\n");
        return;
    }
    release(id);
}

void remove(char const * name) {
    if (!is_valid_string(name)) {
        ipc::error("fail remove: name is empty\n");
        return;
    }
    // Do Nothing.
}

} // namespace shm
} // namespace ipc

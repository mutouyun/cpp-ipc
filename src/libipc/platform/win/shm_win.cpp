
#if defined(__MINGW32__)
#include <windows.h>
#else
#include <Windows.h>
#endif

#include <atomic>
#include <string>
#include <utility>

#include "libipc/shm.h"
#include "libipc/def.h"
#include "libipc/pool_alloc.h"

#include "libipc/utility/log.h"
#include "libipc/mem/resource.h"

#include "to_tchar.h"
#include "get_sa.h"

namespace {

struct info_t {
    std::atomic<std::int32_t> acc_;
};

struct id_info_t {
    HANDLE      h_    = NULL;
    void*       mem_  = nullptr;
    std::size_t size_ = 0;
};

constexpr std::size_t calc_size(std::size_t size) {
    return ((((size - 1) / alignof(info_t)) + 1) * alignof(info_t)) + sizeof(info_t);
}

inline auto& acc_of(void* mem, std::size_t size) {
    return reinterpret_cast<info_t*>(static_cast<ipc::byte_t*>(mem) + size - sizeof(info_t))->acc_;
}

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
        std::size_t alloc_size = calc_size(size);
        h = ::CreateFileMapping(INVALID_HANDLE_VALUE, detail::get_sa(), PAGE_READWRITE | SEC_COMMIT,
                                0, static_cast<DWORD>(alloc_size), fmt_name.c_str());
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

std::int32_t get_ref(id_t id) {
    if (id == nullptr) {
        return 0;
    }
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ == nullptr || ii->size_ == 0) {
        return 0;
    }
    return acc_of(ii->mem_, calc_size(ii->size_)).load(std::memory_order_acquire);
}

void sub_ref(id_t id) {
    if (id == nullptr) {
        ipc::error("fail sub_ref: invalid id (null)\n");
        return;
    }
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ == nullptr || ii->size_ == 0) {
        ipc::error("fail sub_ref: invalid id (mem = %p, size = %zd)\n", ii->mem_, ii->size_);
        return;
    }
    acc_of(ii->mem_, calc_size(ii->size_)).fetch_sub(1, std::memory_order_acq_rel);
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
    std::size_t actual_size = static_cast<std::size_t>(mem_info.RegionSize);
    if (ii->size_ == 0) {
        // Opening existing shared memory
        ii->size_ = actual_size - sizeof(info_t);
    }
    // else: Keep user-requested size (already set in acquire)
    ii->mem_ = mem;
    if (size != nullptr) *size = ii->size_;
    // Initialize or increment reference counter
    acc_of(mem, calc_size(ii->size_)).fetch_add(1, std::memory_order_release);
    return static_cast<void *>(mem);
}

std::int32_t release(id_t id) noexcept {
    if (id == nullptr) {
        ipc::error("fail release: invalid id (null)\n");
        return -1;
    }
    std::int32_t ret = -1;
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ == nullptr || ii->size_ == 0) {
        ipc::error("fail release: invalid id (mem = %p, size = %zd)\n", ii->mem_, ii->size_);
    }
    else {
        ret = acc_of(ii->mem_, calc_size(ii->size_)).fetch_sub(1, std::memory_order_acq_rel);
        ::UnmapViewOfFile(static_cast<LPCVOID>(ii->mem_));
    }
    if (ii->h_ == NULL) {
        ipc::error("fail release: invalid id (h = null)\n");
    }
    else ::CloseHandle(ii->h_);
    mem::free(ii);
    return ret;
}

void remove(id_t id) noexcept {
    if (id == nullptr) {
        ipc::error("fail release: invalid id (null)\n");
        return;
    }
    release(id);
}

void remove(char const * name) noexcept {
    if (!is_valid_string(name)) {
        ipc::error("fail remove: name is empty\n");
        return;
    }
    // Do Nothing.
}

} // namespace shm
} // namespace ipc

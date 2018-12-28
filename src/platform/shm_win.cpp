#include "shm.h"

#include <Windows.h>

#include <type_traits>
#include <string>
#include <locale>
#include <codecvt>
#include <utility>
#include <unordered_map>

#include "def.h"
#include "tls_pointer.h"

namespace {

template <typename T, typename S, typename R = S>
using IsSame = ipc::Requires<std::is_same<T, typename S::value_type>::value, R>;

template <typename T = TCHAR>
constexpr auto to_tchar(std::string && str) -> IsSame<T, std::string, std::string &&> {
    return std::move(str);
}

template <typename T = TCHAR>
constexpr auto to_tchar(std::string && str) -> IsSame<T, std::wstring> {
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{}.from_bytes(std::move(str));
}

inline auto& m2h() {
    static ipc::tls::pointer<std::unordered_map<void*, HANDLE>> cache;
    return *cache.create();
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
                                   to_tchar(std::string{"__SHM__"} + name).c_str());
    if (h == NULL) {
        return nullptr;
    }
    LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (mem == NULL) {
        ::CloseHandle(h);
        return nullptr;
    }
    m2h().emplace(mem, h);
    return mem;
}

void release(void* mem, std::size_t /*size*/) {
    if (mem == nullptr) {
        return;
    }
    auto& cc = m2h();
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

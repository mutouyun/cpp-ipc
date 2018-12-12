#include "shm.h"

#include <windows.h>

#include <type_traits>
#include <string>
#include <locale>
#include <codecvt>
#include <utility>

namespace {

template <typename T, typename S, typename R = S>
using IsSame = std::enable_if_t<std::is_same<T, typename S::value_type>::value, R>;

template <typename T = TCHAR>
constexpr auto to_tchar(std::string && str) -> IsSame<T, std::string, std::string &&> {
    return std::move(str);
}

template <typename T = TCHAR>
inline auto to_tchar(std::string && str) -> IsSame<T, std::wstring> {
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{}.from_bytes(std::move(str));
}

} // internal-linkage

namespace ipc {
namespace shm {

handle_t acquire(char const * name, std::size_t size) {
    if (name == nullptr || name[0] == '\0' || size == 0) {
        return nullptr;
    }
    HANDLE h = ::CreateFileMapping(
        INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE | SEC_COMMIT,
        0, static_cast<DWORD>(size),
        to_tchar(std::string{"__SHM__"} + name).c_str()
    );
    if (h == NULL) {
        return nullptr;
    }
    return h;
}

void release(handle_t h, std::size_t /*size*/) {
    if (h == nullptr) {
        return;
    }
    ::CloseHandle(h);
}

void* open(handle_t h) {
    if (h == nullptr) {
        return nullptr;
    }
    LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (mem == NULL) {
        return nullptr;
    }
    return mem;
}

void close(void* mem) {
    if (mem == nullptr) {
        return;
    }
    ::UnmapViewOfFile(mem);
}

} // namespace shm
} // namespace ipc

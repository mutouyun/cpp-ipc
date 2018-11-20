#include "shm.h"

#include <windows.h>

#include <type_traits>
#include <locale>
#include <codecvt>

namespace {

template <typename T = TCHAR>
constexpr auto to_tchar(std::string const & str)
 -> std::enable_if_t<std::is_same<std::string::value_type, T>::value, std::string const &> {
    return str;
}

template <typename T = TCHAR>
inline auto to_tchar(std::string const & str)
 -> std::enable_if_t<std::is_same<std::wstring::value_type, T>::value, std::wstring> {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

} // internal-linkage

namespace ipc {
namespace shm {

handle_t acquire(std::string const & name, std::size_t size) {
    if (name.empty() || size == 0) {
        return nullptr;
    }
    HANDLE h = ::CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE | SEC_COMMIT,
        0, static_cast<DWORD>(size),
        to_tchar("__SHM__" + name).c_str()
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

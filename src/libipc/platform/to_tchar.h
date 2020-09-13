#pragma once

#include <tchar.h>

#include <type_traits>
#include <string>
#include <locale>
#include <codecvt>
#include <cstring>

#include "libipc/utility/concept.h"
#include "libipc/platform/detail.h"
#include "libipc/memory/resource.h"

namespace ipc {
namespace detail {

struct has_value_type_ {
    template <typename T> static std::true_type  check(typename T::value_type *);
    template <typename T> static std::false_type check(...);
};

template <typename T, typename U, typename = decltype(has_value_type_::check<U>(nullptr))>
struct is_same_char : std::is_same<T, U> {};

template <typename T, typename U>
struct is_same_char<T, U, std::true_type> : std::is_same<T, typename U::value_type> {};

template <typename T, typename S, typename R = S>
using IsSameChar = ipc::require<is_same_char<T, S>::value, R>;

////////////////////////////////////////////////////////////////
/// to_tchar implementation
////////////////////////////////////////////////////////////////

template <typename T = TCHAR>
constexpr auto to_tchar(ipc::string && str) -> IsSameChar<T, ipc::string, ipc::string &&> {
    return std::move(str);
}

template <typename T = TCHAR>
constexpr auto to_tchar(ipc::string && str) -> IsSameChar<T, ipc::wstring> {
    return std::wstring_convert<
        std::codecvt_utf8_utf16<wchar_t>,
        wchar_t,
        ipc::mem::allocator<wchar_t>,
        ipc::mem::allocator<char>
    >{}.from_bytes(std::move(str));
}

template <typename T>
inline auto to_tchar(T* dst, char const * src, std::size_t size) -> IsSameChar<T, char, void> {
    std::memcpy(dst, src, size);
}

template <typename T>
inline auto to_tchar(T* dst, char const * src, std::size_t size) -> IsSameChar<T, wchar_t, void> {
    auto wstr = std::wstring_convert<
        std::codecvt_utf8_utf16<wchar_t>,
        wchar_t,
        ipc::mem::allocator<wchar_t>,
        ipc::mem::allocator<char>
    >{}.from_bytes(src, src + size);
    std::memcpy(dst, wstr.data(), (ipc::detail::min)(wstr.size(), size));
}

} // namespace detail
} // namespace ipc

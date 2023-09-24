#pragma once

#include <Windows.h>

#include <type_traits>
#include <string>
#include <locale>
#include <codecvt>
#include <cstring>
#include <stdexcept>
#include <cstddef>

#include "libipc/utility/concept.h"
#include "libipc/memory/resource.h"
#include "libipc/platform/detail.h"

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
constexpr auto to_tchar(ipc::string &&str) -> IsSameChar<T, ipc::string, ipc::string &&> {
    return std::move(str); // noconv
}

/**
 * \remarks codecvt_utf8_utf16/std::wstring_convert is deprecated
 * \see https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/
 *      https://stackoverflow.com/questions/42946335/deprecated-header-codecvt-replacement
 *      https://en.cppreference.com/w/cpp/locale/codecvt/in
 *      https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
*/
template <typename T = TCHAR>
auto to_tchar(ipc::string &&external) -> IsSameChar<T, ipc::wstring> {
    if (external.empty()) {
        return {}; // noconv
    }
    /**
     * CP_ACP       : The system default Windows ANSI code page.
     * CP_MACCP     : The current system Macintosh code page.
     * CP_OEMCP     : The current system OEM code page.
     * CP_SYMBOL    : Symbol code page (42).
     * CP_THREAD_ACP: The Windows ANSI code page for the current thread.
     * CP_UTF7      : UTF-7. Use this value only when forced by a 7-bit transport mechanism. Use of UTF-8 is preferred.
     * CP_UTF8      : UTF-8.
    */
    int size_needed = ::MultiByteToWideChar(CP_UTF8, 0, &external[0], (int)external.size(), NULL, 0);
    if (size_needed <= 0) {
        return {};
    }
    ipc::wstring internal(size_needed, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, &external[0], (int)external.size(), &internal[0], size_needed);
    return internal;
}

} // namespace detail
} // namespace ipc

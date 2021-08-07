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
constexpr auto to_tchar(ipc::string &&str) -> IsSameChar<T, ipc::string, ipc::string &&> {
    return std::move(str); // noconv
}

/**
 * codecvt_utf8_utf16/std::wstring_convert is deprecated
 * @see https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/
 *      https://stackoverflow.com/questions/42946335/deprecated-header-codecvt-replacement
 *      https://en.cppreference.com/w/cpp/locale/codecvt/in
 *      https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
*/
template <typename T = TCHAR>
auto to_tchar(ipc::string &&external) -> IsSameChar<T, ipc::wstring> {
    if (external.empty()) {
        return {}; // noconv
    }
#if 0 // backup
    auto &fcodecvt = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(std::locale());
    std::mbstate_t mb {};
    ipc::wstring internal(external.size(), '\0');
    char const *first = &external[0], *last = &external[external.size()];
    std::size_t len   = 0;
    while (first != last) {
        wchar_t *start = &internal[len], *end = &internal[internal.size()], *to_next;
        auto ret = fcodecvt.in(mb, first, last, first,
                                   start, end , to_next);
        switch (ret) {
        // no conversion, just copy code values
        case std::codecvt_base::noconv:
            internal.resize(len);
            for (; first != last; ++first) {
                internal.push_back((wchar_t)(unsigned char)*first);
            }
            break;
        // conversion completed
        case std::codecvt_base::ok:
            len += static_cast<size_t>(to_next - start);
            internal.resize(len);
            break;
        // not enough space in the output buffer or unexpected end of source buffer
        case std::codecvt_base::partial:
            if (to_next <= start) {
                throw std::range_error{"[to_tchar] bad conversion"};
            }
            len += static_cast<size_t>(to_next - start);
            internal.resize(internal.size() + external.size());
            break;
        // encountered a character that could not be converted
        default: // error
            throw std::range_error{"[to_tchar] bad conversion"};
        }
    }
#else
    // CP_ACP: The system default Windows ANSI code page.
    int size_needed = ::MultiByteToWideChar(CP_ACP, 0, &external[0], (int)external.size(), NULL, 0);
    if (size_needed <= 0) {
        return {};
    }
    ipc::wstring internal(size_needed, L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, &external[0], (int)external.size(), &internal[0], size_needed);
#endif
    return internal;
}

} // namespace detail
} // namespace ipc

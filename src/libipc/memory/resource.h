#pragma once

#include <type_traits>
#include <limits>
#include <utility>
#include <functional>
#include <unordered_map>
#include <map>
#include <string>
#include <cstdio>

#include "libipc/def.h"

#include "libipc/memory/alloc.h"
#include "libipc/memory/wrapper.h"
#include "libipc/platform/detail.h"
#include "libipc/imp/fmt.h"

namespace ipc {
namespace mem {

//using async_pool_alloc = static_wrapper<variable_wrapper<async_wrapper<
//    detail::fixed_alloc<
//        variable_alloc                           <sizeof(void*) * 1024 * 256>, 
//        fixed_expand_policy<sizeof(void*) * 1024, sizeof(void*) * 1024 * 256>
//    >, 
//    default_recycler >>>;
using async_pool_alloc = ipc::mem::static_alloc;

template <typename T>
using allocator = allocator_wrapper<T, async_pool_alloc>;

} // namespace mem

template <typename T>
struct hash : public std::hash<T> {};

template <typename Key, typename T>
using unordered_map = std::unordered_map<
    Key, T, ipc::hash<Key>, std::equal_to<Key>, ipc::mem::allocator<std::pair<Key const, T>>
>;

template <typename Key, typename T>
using map = std::map<
    Key, T, std::less<Key>, ipc::mem::allocator<std::pair<Key const, T>>
>;

template <typename Char>
using basic_string = std::basic_string<
    Char, std::char_traits<Char>, ipc::mem::allocator<Char>
>;

using string  = basic_string<char>;
using wstring = basic_string<wchar_t>;

template <> struct hash<string> {
    std::size_t operator()(string const &val) const noexcept {
        return std::hash<char const *>{}(val.c_str());
    }
};

template <> struct hash<wstring> {
    std::size_t operator()(wstring const &val) const noexcept {
        return std::hash<wchar_t const *>{}(val.c_str());
    }
};

/// \brief Check string validity.
constexpr bool is_valid_string(char const *str) noexcept {
    return (str != nullptr) && (str[0] != '\0');
}

/// \brief Make a valid string.
inline std::string make_string(char const *str) {
    return is_valid_string(str) ? std::string{str} : std::string{};
}

/// \brief Combine prefix from a list of strings.
template <typename... A>
inline std::string make_prefix(std::string prefix, A &&...args) {
    return ipc::fmt(prefix, "__IPC_SHM__", std::forward<A>(args)...);
}

} // namespace ipc

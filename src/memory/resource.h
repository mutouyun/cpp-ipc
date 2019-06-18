#pragma once

#include <type_traits>
#include <limits>
#include <utility>
#include <functional>
#include <unordered_map>
#include <string>
#include <cstdio>

#include "def.h"

#include "memory/alloc.h"
#include "memory/wrapper.h"
#include "memory/detail.h"
#include "platform/detail.h"

namespace ipc {
namespace mem {

template <std::size_t Size>
using sync_fixed_alloc = mem::synchronized<page_fixed_alloc<Size>>;

template <std::size_t Size>
using sync_fixed = mem::detail::fixed<Size, sync_fixed_alloc>;

using sync_pool_alloc = mem::detail::pool_alloc<sync_fixed>;

template <typename T>
using allocator = allocator_wrapper<T, sync_pool_alloc>;

} // namespace mem

namespace {

constexpr char const * pf(int)                { return "%d"  ; }
constexpr char const * pf(long)               { return "%ld" ; }
constexpr char const * pf(long long)          { return "%lld"; }
constexpr char const * pf(unsigned int)       { return "%u"  ; }
constexpr char const * pf(unsigned long)      { return "%lu" ; }
constexpr char const * pf(unsigned long long) { return "%llu"; }
constexpr char const * pf(float)              { return "%f"  ; }
constexpr char const * pf(double)             { return "%f"  ; }
constexpr char const * pf(long double)        { return "%Lf" ; }

} // internal-linkage

template <typename Key, typename T>
using unordered_map = std::unordered_map<
    Key, T, std::hash<Key>, std::equal_to<Key>, ipc::mem::allocator<std::pair<const Key, T>>
>;

using string = std::basic_string<
    char, std::char_traits<char>, ipc::mem::allocator<char>
>;

using wstring = std::basic_string<
    wchar_t, std::char_traits<wchar_t>, ipc::mem::allocator<wchar_t>
>;

template <typename T>
ipc::string to_string(T val) {
    char buf[std::numeric_limits<T>::digits10 + 1] {};
    if (std::snprintf(buf, sizeof(buf), pf(val), val) > 0) {
        return buf;
    }
    return {};
}

} // namespace ipc

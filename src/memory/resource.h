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
using static_async_fixed = mem::static_wrapper<mem::async_wrapper<mem::fixed_alloc<Size>>>;
using async_pool_alloc   = mem::detail::fixed_alloc_policy<static_async_fixed>;

template <typename T>
using allocator = allocator_wrapper<T, async_pool_alloc>;

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

template <typename Char>
using basic_string = std::basic_string<
    Char, std::char_traits<Char>, ipc::mem::allocator<Char>
>;

using string  = basic_string<char>;
using wstring = basic_string<wchar_t>;

template <typename T>
ipc::string to_string(T val) {
    char buf[std::numeric_limits<T>::digits10 + 1] {};
    if (std::snprintf(buf, sizeof(buf), pf(val), val) > 0) {
        return buf;
    }
    return {};
}

} // namespace ipc

#pragma once

#include <type_traits>
#include <limits>
#include <utility>
#include <functional>
#include <unordered_map>
#include <vector>

#include "def.h"

#include "memory/alloc.hpp"
#include "memory/wrapper.hpp"
#include "memory/detail.h"
#include "platform/detail.h"

namespace ipc {
namespace mem {

template <std::size_t Size>
using sync_fixed_alloc = synchronized<page_fixed_alloc<Size>>;

template <std::size_t Size>
using sync_fixed = mem::detail::fixed<Size, sync_fixed_alloc>;

using sync_pool_alloc = detail::pool_alloc<sync_fixed>;

template <typename T>
using allocator = allocator_wrapper<T, sync_pool_alloc>;

template <typename Key, typename T>
using unordered_map = std::unordered_map<
    Key, T, std::hash<Key>, std::equal_to<Key>, allocator<std::pair<const Key, T>>
>;

template <typename T>
using vector = std::vector<T, allocator<T>>;

} // namespace mem
} // namespace ipc

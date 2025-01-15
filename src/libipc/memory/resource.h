#pragma once

#include <unordered_map>
#include <map>
#include <string>

#include "libipc/def.h"
#include "libipc/memory/alloc.h"
#include "libipc/imp/fmt.h"
#include "libipc/mem/polymorphic_allocator.h"

namespace ipc {
namespace mem {

//using async_pool_alloc = static_wrapper<variable_wrapper<async_wrapper<
//    detail::fixed_alloc<
//        variable_alloc                           <sizeof(void*) * 1024 * 256>, 
//        fixed_expand_policy<sizeof(void*) * 1024, sizeof(void*) * 1024 * 256>
//    >, 
//    default_recycler >>>;
using async_pool_alloc = ipc::mem::static_alloc;

} // namespace mem

template <typename T>
struct hash : public std::hash<T> {};

template <typename Key, typename T>
using unordered_map = std::unordered_map<
  Key, T, ipc::hash<Key>, std::equal_to<Key>, ipc::mem::polymorphic_allocator<std::pair<Key const, T>>
>;

template <typename Key, typename T>
using map = std::map<
  Key, T, std::less<Key>, ipc::mem::polymorphic_allocator<std::pair<Key const, T>>
>;

/// \brief Check string validity.
constexpr bool is_valid_string(char const *str) noexcept {
  return (str != nullptr) && (str[0] != '\0');
}

/// \brief Make a valid string.
inline std::string make_string(char const *str) {
  return is_valid_string(str) ? std::string{str} : std::string{};
}

/// \brief Combine prefix from a list of strings.
template <typename A1, typename... A>
inline std::string make_prefix(A1 &&prefix, A &&...args) {
  return ipc::fmt(std::forward<A1>(prefix), "__IPC_SHM__", std::forward<A>(args)...);
}

} // namespace ipc

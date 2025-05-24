#pragma once

#include <unordered_map>
#include <map>
#include <string>

#include "libipc/def.h"
#include "libipc/imp/fmt.h"
#include "libipc/mem/container_allocator.h"

namespace ipc {

template <typename Key, typename T>
using unordered_map = std::unordered_map<
  Key, T, std::hash<Key>, std::equal_to<Key>, ipc::mem::container_allocator<std::pair<Key const, T>>
>;

template <typename Key, typename T>
using map = std::map<
  Key, T, std::less<Key>, ipc::mem::container_allocator<std::pair<Key const, T>>
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

/**
 * @file src/pimpl.h
 * @author mutouyun (orz@orzz.org)
 * @brief Pointer To Implementation (pImpl) idiom
 * @date 2022-02-27
 */
#pragma once

#include <utility>
#include <type_traits>
#include <cstdint>

#include "libipc/utility/construct.h"
#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_
namespace pimpl {

template <typename T, typename R = T *>
struct is_comfortable {
  enum : bool {
    value = (sizeof(T) <= sizeof(R)) && (alignof(T) <= alignof(R))
  };
};

template <typename T, typename... A>
auto make(A &&... args) -> std::enable_if_t<is_comfortable<T>::value, T *> {
  T *buf {};
  // construct an object using memory of a pointer
  construct<T>(&buf, std::forward<A>(args)...);
  return buf;
}

template <typename T>
auto get(T * const (& p)) noexcept -> std::enable_if_t<is_comfortable<T>::value, T *> {
  return reinterpret_cast<T*>(&const_cast<char &>(reinterpret_cast<char const &>(p)));
}

template <typename T>
auto clear(T *p) noexcept -> std::enable_if_t<is_comfortable<T>::value> {
  if (p != nullptr) destroy(get(p));
}

template <typename T, typename... A>
auto make(A &&... args) -> std::enable_if_t<!is_comfortable<T>::value, T *> {
  return new T{std::forward<A>(args)...};
}

template <typename T>
auto get(T * const (& p)) noexcept -> std::enable_if_t<!is_comfortable<T>::value, T *> {
  return p;
}

template <typename T>
auto clear(T *p) noexcept -> std::enable_if_t<!is_comfortable<T>::value> {
  if (p != nullptr) delete p;
}

template <typename T>
class Obj {
 public:
  template <typename... A>
  static T *make(A &&... args) {
    return pimpl::make<T>(std::forward<A>(args)...);
  }

  void clear() noexcept {
    pimpl::clear(static_cast<T *>(const_cast<Obj *>(this)));
  }
};

} // namespace pimpl
LIBIPC_NAMESPACE_END_

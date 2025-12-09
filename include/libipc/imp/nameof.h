/**
 * \file libipc/nameof.h
 * \author mutouyun (orz@orzz.org)
 * \brief Gets the name string of a type.
 */
#pragma once

#include <typeinfo>
#include <string>
#include <cstring>

#include "libipc/imp/export.h"
#include "libipc/imp/span.h"
#include "libipc/imp/detect_plat.h"

namespace ipc {

/**
 * \brief The conventional way to obtain demangled symbol name.
 * \see https://www.boost.org/doc/libs/1_80_0/libs/core/doc/html/core/demangle.html
 * 
 * \param name the mangled name
 * \return std::string a human-readable demangled type name
 */
LIBIPC_EXPORT std::string demangle(std::string name) noexcept;

/**
 * \brief Returns an implementation defined string containing the name of the type.
 * \see https://en.cppreference.com/w/cpp/types/type_info/name
 * 
 * \tparam T a type
 * \return std::string a human-readable demangled type name
 */
template <typename T>
std::string nameof() noexcept {
  LIBIPC_TRY {
    return demangle(typeid(T).name());
  } LIBIPC_CATCH(...) {
    return {};
  }
}

} // namespace ipc

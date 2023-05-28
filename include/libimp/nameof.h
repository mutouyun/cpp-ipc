/**
 * \file libimp/nameof.h
 * \author mutouyun (orz@orzz.org)
 * \brief Gets the name string of a type.
 * \date 2022-11-26
 */
#pragma once

#include <typeinfo>
#include <string>
#include <cstring>

#include "libimp/def.h"
#include "libimp/export.h"
#include "libimp/span.h"

LIBIMP_NAMESPACE_BEG_

/**
 * \brief The conventional way to obtain demangled symbol name.
 * \see https://www.boost.org/doc/libs/1_80_0/libs/core/doc/html/core/demangle.html
 * 
 * \param name the mangled name
 * \return std::string a human-readable demangled type name
 */
LIBIMP_EXPORT std::string demangle(span<char const> name) noexcept;

/**
 * \brief Returns an implementation defined string containing the name of the type.
 * \see https://en.cppreference.com/w/cpp/types/type_info/name
 * 
 * \tparam T a type
 * \return std::string a human-readable demangled type name
 */
template <typename T>
std::string nameof() noexcept {
  auto c_str_name = typeid(T).name();
  return demangle({c_str_name, std::strlen(c_str_name)});
}

LIBIMP_NAMESPACE_END_

/**
 * @file libimp/generic.h
 * @author mutouyun (orz@orzz.org)
 * @brief Tools for generic programming
 * @date 2022-03-01
 */
#pragma once

#include "libimp/def.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief Utility metafunction that maps a sequence of any types to the type void
 * @see https://en.cppreference.com/w/cpp/types/void_t
*/
template <typename...>
using void_t = void;

LIBIMP_NAMESPACE_END_

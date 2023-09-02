/**
 * \file libpmr/small_storage.h
 * \author mutouyun (orz@orzz.org)
 * \brief Unified SSO (Small Size Optimization).
 * \date 2023-09-02
 */
#pragma once

#include <type_traits>
#include <array>
#include <typeinfo>
#include <utility>
#include <memory>
#include <cstddef>

#include "libimp/export.h"
#include "libimp/construct.h"
#include "libimp/byte.h"
#include "libimp/generic.h"

#include "libpmr/def.h"
#include "libpmr/holder_base.h"

LIBPMR_NAMESPACE_BEG_

/**
 * \class template <typename Value> holder<Value, false>
 * \brief A holder implementation that holds a data object if type `Value` on heap memory.
 * \tparam Value The storage type of the holder.
 */
template <typename Value>
class holder<Value, false> : public holder_base {
};

LIBPMR_NAMESPACE_END_

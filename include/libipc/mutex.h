/**
 * \file libipc/mutex.h
 * \author mutouyun (orz@orzz.org)
 * \brief A synchronization primitive that can be used to protect shared data 
 *        from being simultaneously accessed by multiple processes.
 */
#pragma once

#include <string>
#include <cstddef>
#include <cstdint>

#include "libimp/export.h"
#include "libimp/result.h"
#include "libimp/span.h"
#include "libimp/byte.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

struct mutex_handle;
using mutex_t = mutex_handle *;

/// \brief Create or open a mutex object based on memory.
LIBIMP_EXPORT ::LIBIMP::result<mutex_t> mutex_open(::LIBIMP::span<::LIBIMP::byte>) noexcept;

/// \brief Close the mutex handle.
/// \note The mutex object will be destroyed when the last handle is closed,
///       and the lifetime of a mutex object needs to be shorter than 
///       the memory specified when it is created or opened.
LIBIMP_EXPORT ::LIBIMP::result<void> mutex_close(mutex_t) noexcept;

/// \brief Gets the memory size of the specified mutex.
LIBIMP_EXPORT ::LIBIMP::result<std::size_t> mutex_size(mutex_t) noexcept;

/// \brief Locks the mutex, blocks if the mutex is not available.
LIBIMP_EXPORT ::LIBIMP::result<bool> mutex_lock(mutex_t, std::int64_t ms) noexcept;

/// \brief Tries to lock the mutex, returns if the mutex is not available.
LIBIMP_EXPORT ::LIBIMP::result<bool> mutex_try_lock(mutex_t) noexcept;

/// \brief Unlocks the mutex.
LIBIMP_EXPORT ::LIBIMP::result<bool> mutex_unlock(mutex_t) noexcept;

LIBIPC_NAMESPACE_END_

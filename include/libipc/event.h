/**
 * \file libipc/event.h
 * \author mutouyun (orz@orzz.org)
 * \brief Defines event objects for cross-process communication.
 */
#pragma once

#include <string>
#include <cstdint>

#include "libimp/export.h"
#include "libimp/result.h"
#include "libimp/span.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

struct evt_handle;
using evt_t = evt_handle *;

/// \brief Create a new event object with a name.
LIBIMP_EXPORT ::LIBIMP::result<evt_t> evt_open(std::string name) noexcept;

/// \brief Close the event object.
LIBIMP_EXPORT ::LIBIMP::result<void> evt_close(evt_t) noexcept;

/// \brief Gets the name of the event object based on the event handle.
/// \return empty string on failure.
LIBIMP_EXPORT std::string evt_name(evt_t) noexcept;

/// \brief Sets the event object to the signaled state.
LIBIMP_EXPORT ::LIBIMP::result<void> evt_set(evt_t) noexcept;

/// \brief Waits until one of the specified event objects is in the signaled state.
LIBIMP_EXPORT ::LIBIMP::result<bool> evt_wait(evt_t, std::int64_t ms) noexcept;
LIBIMP_EXPORT ::LIBIMP::result<bool> evt_wait(::LIBIMP::span<evt_t>, std::int64_t ms) noexcept;

/**
 * \brief The event object.
 */
class LIBIMP_EXPORT event {
  evt_t evt_;
};

LIBIPC_NAMESPACE_END_

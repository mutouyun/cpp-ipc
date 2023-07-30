/**
 * \file libipc/platform/win/event_impl.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstddef>

#include <Windows.h>

#include "libimp/log.h"
#include "libipc/event.h"

#include "get_sa.h"
#include "to_tchar.h"

LIBIPC_NAMESPACE_BEG_
using namespace ::LIBIMP;

struct evt_handle {
  std::string name;
  HANDLE h_event;
};

namespace {

bool is_valid(evt_t evt) noexcept {
  return (evt != nullptr) && (evt->h_event != NULL);
}

} // namespace

/**
 * \brief Creates or opens a named event object.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventa
 * \param name The name of the event object. The name is limited to MAX_PATH characters. Name comparison is case sensitive.
 * \return A handle to the event object, NULL on error.
 */
result<evt_t> evt_open(std::string name) noexcept {
  LIBIMP_LOG_();
  auto t_name = detail::to_tstring(name);
  auto h = ::CreateEvent(detail::get_sa(), 
                        /*bManualReset*/ FALSE, 
                        /*bInitialState*/FALSE, 
                        /*lpName*/       t_name.c_str());
  if (h == NULL) {
    auto err = sys::error();
    log.error("failed: CreateEvent(FALSE, FALSE, ", name, "). error = ", err);
    return {nullptr, err};
  }
  return new evt_handle{std::move(name), h};
}

/**
 * \brief Closes an open object handle.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
 */
result<void> evt_close(evt_t evt) noexcept {
  LIBIMP_LOG_();
  LIBIMP_UNUSED auto guard = std::unique_ptr<evt_handle>(evt);
  if (!is_valid(evt)) {
    log.error("handle is null.");
    return {};
  }
  if (!::CloseHandle(evt->h_event)) {
    auto err = sys::error();
    log.error("failed: CloseHandle(", evt->h_event, "). error = ", err);
    return err;
  }
  return no_error;
}

/**
 * \brief Sets the specified event object to the signaled state.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
 */
result<void> evt_set(evt_t evt) noexcept {
  LIBIMP_LOG_();
  if (!is_valid(evt)) {
    log.error("handle is null.");
    return {};
  }
  if (!::SetEvent(evt->h_event)) {
    auto err = sys::error();
    log.error("failed: SetEvent(", evt->h_event, "). error = ", err);
    return err;
  }
  return no_error;
}

/**
 * \brief Waits until the specified object is in the signaled state or the time-out interval elapses.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
 */
result<bool> evt_wait(evt_t evt, std::int64_t ms) noexcept {
  LIBIMP_LOG_();
  if (!is_valid(evt)) {
    log.error("handle is null.");
    return {};
  }
  DWORD dwMilliseconds = (ms < 0) ? INFINITE : static_cast<DWORD>(ms);
  auto r = ::WaitForSingleObject(evt->h_event, dwMilliseconds);
  if (r == WAIT_TIMEOUT) {
    return false;
  }
  if (r == WAIT_ABANDONED_0) {
    log.error("failed: WaitForSingleObject(", evt->h_event, ", ", dwMilliseconds, "). error = WAIT_ABANDONED_0");
    return {};
  }
  if (r == WAIT_OBJECT_0) {
    return true;
  }
  auto err = sys::error();
  log.error("failed: WaitForSingleObject(", evt->h_event, ", ", dwMilliseconds, "). error = ", err);
  return {false, err};
}

/**
 * \brief Waits until one or all of the specified objects are in the signaled state or the time-out interval elapses.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects
 */
result<bool> evt_wait(::LIBIMP::span<evt_t const> evts, std::int64_t ms) noexcept {
  LIBIMP_LOG_();
  if (evts.empty()) {
    log.error("evts handle is empty.");
    return {};
  }
  // Conversion of the event handle array to the windows handle array.
  std::vector<HANDLE> handles(evts.size());
  for (std::size_t i = 0; i < evts.size(); ++i) {
    if (!is_valid(evts[i])) {
      log.error("handle is null.");
      return {};
    }
    handles[i] = evts[i]->h_event;
  }
  // Wait for the events.
  DWORD dwMilliseconds = (ms < 0) ? INFINITE : static_cast<DWORD>(ms);
  auto r = ::WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, dwMilliseconds);
  if (r == WAIT_TIMEOUT) {
    return false;
  }
  if ((r >= WAIT_ABANDONED_0) && (r < WAIT_ABANDONED_0 + handles.size())) {
    log.error("failed: WaitForMultipleObjects(", handles.size(), ", ", dwMilliseconds, "). error = WAIT_ABANDONED_0 + ", r - WAIT_ABANDONED_0);
    return {};
  }
  if ((r >= WAIT_OBJECT_0) && (r < WAIT_OBJECT_0 + handles.size())) {
    return true;
  }
  auto err = sys::error();
  log.error("failed: WaitForMultipleObjects(", handles.size(), ", ", dwMilliseconds, "). error = ", err);
  return {false, err};
}

LIBIPC_NAMESPACE_END_

/**
 * \file libipc/platform/win/api.h
 * \author mutouyun (orz@orzz.org)
 */
#pragma once

#include <Windows.h>
#include <securitybaseapi.h>
#include <tchar.h>

#include <string>
#include <cstddef>

#include "libimp/log.h"
#include "libimp/system.h"
#include "libimp/codecvt.h"
#include "libimp/span.h"
#include "libimp/detect_plat.h"

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_

using namespace ::LIBIMP;

namespace winapi {

using tstring = std::basic_string<TCHAR>;

inline tstring to_tstring(std::string const &str) {
  tstring des;
  ::LIBIMP::cvt_sstr(str, des);
  return des;
}

/**
 * \brief Creates or opens a SECURITY_ATTRIBUTES structure singleton
 * \see https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa379560(v=vs.85)
 */
inline LPSECURITY_ATTRIBUTES get_security_descriptor() {
  static struct initiator {

    SECURITY_DESCRIPTOR sd_;
    SECURITY_ATTRIBUTES sa_;

    bool succ_ = false;

    initiator() {
      using namespace ::LIBIMP;
      LIBIMP_LOG_("get_security_descriptor");
      if (!::InitializeSecurityDescriptor(&sd_, SECURITY_DESCRIPTOR_REVISION)) {
        log.error("failed: InitializeSecurityDescriptor(SECURITY_DESCRIPTOR_REVISION). "
                  "error = ", sys::error());
        return;
      }
      if (!::SetSecurityDescriptorDacl(&sd_, TRUE, NULL, FALSE)) {
        log.error("failed: SetSecurityDescriptorDacl. error = ", sys::error());
        return;
      }
      sa_.nLength = sizeof(SECURITY_ATTRIBUTES);
      sa_.bInheritHandle = FALSE;
      sa_.lpSecurityDescriptor = &sd_;
      succ_ = true;
    }
  } handle;
  return handle.succ_ ? &handle.sa_ : nullptr;
}

/**
 * \brief Closes an open object handle.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
 */
inline result<void> close_handle(HANDLE h) noexcept {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  if (!::CloseHandle(h)) {
    auto err = sys::error();
    log.error("failed: CloseHandle(", h, "). error = ", err);
    return err;
  }
  return std::error_code{};
}

/**
 * \brief Creates or opens a file mapping object for a specified file.
 * 
 * \see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilemappinga
 *      https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga
 * 
 * \param file Specifies the name of the file mapping object.
 * \param size Specifies the size required to create a file mapping object.
 *             This size is ignored when opening an existing file mapping object.
 * \param type Combinable open modes, create | open.
 * 
 * \return File mapping object HANDLE, NULL on error.
 */
inline result<HANDLE> open_file_mapping(std::string const &file, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  if (file.empty()) {
    log.error("file name is empty.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  auto t_name = winapi::to_tstring(file);
  if (t_name.empty()) {
    log.error("file name is empty. (TCHAR conversion failed)");
    return std::make_error_code(std::errc::invalid_argument);
  }

  // Opens a named file mapping object.
  auto try_open = [&]() -> result<HANDLE> {
    HANDLE h = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, t_name.c_str());
    if (h == NULL) {
      auto err = sys::error();
      log.error("failed: OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, ", file, "). error = ", err);
      return err;
    }
    return h;
  };

  // Creates or opens a named or unnamed file mapping object for a specified file.
  auto try_create = [&]() -> result<HANDLE> {
    HANDLE h = ::CreateFileMapping(INVALID_HANDLE_VALUE, winapi::get_security_descriptor(), PAGE_READWRITE | SEC_COMMIT,
                                   /// \remark dwMaximumSizeHigh always 0 here.
                                   0, static_cast<DWORD>(size), t_name.c_str());
    if (h == NULL) {
      auto err = sys::error();
      log.error("failed: CreateFileMapping(PAGE_READWRITE | SEC_COMMIT, ", size, ", ", file, "). error = ", err);
      return err;
    }
    return h;
  };

  if (type == mode::open) {
    return try_open();
  } else if ((type == (mode::create | mode::open)) && (size == 0)) {
    /// \remark CreateFileMapping may returns ERROR_INVALID_PARAMETER when dwMaximumSizeLow is zero.
    /// \see CreateFileMapping (Windows CE 5.0)
    ///      https://learn.microsoft.com/en-us/previous-versions/windows/embedded/aa517331(v=msdn.10)
    return try_open();
  } else if (!(type & mode::create)) {
    log.error("mode type is invalid. type = ", type);
    return std::make_error_code(std::errc::invalid_argument);
  }
  auto h = try_create();
  if (!h) return h;
  /// \remark If the object exists before the function call, the function returns a handle to the existing object
  ///         (with its current size, not the specified size), and GetLastError returns ERROR_ALREADY_EXISTS.
  if ((type == mode::create) && (::GetLastError() == ERROR_ALREADY_EXISTS)) {
    log.info("the file being created already exists. file = ", file, ", type = ", type);
    winapi::close_handle(*h);
    return sys::error();
  }
  return h;
}

/**
 * \brief Maps a view of a file mapping into the address space of a calling process.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile
 */
inline result<LPVOID> address_of_file_mapping(HANDLE h) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (mem == NULL) {
    auto err = sys::error();
    log.error("failed: MapViewOfFile(", h, ", FILE_MAP_ALL_ACCESS). error = ", err);
    return err;
  }
  return mem;
}

/**
 * \brief Retrieves the size about a range of pages in the virtual address space of the calling process.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualquery
 */
inline result<SIZE_T> region_size_of_address(LPCVOID mem) {
  LIBIMP_LOG_();
  if (mem == NULL) {
    log.error("memory pointer is null.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  MEMORY_BASIC_INFORMATION mem_info {};
  if (::VirtualQuery(mem, &mem_info, sizeof(mem_info)) == 0) {
    auto err = sys::error();
    log.error("failed: VirtualQuery(", mem, "). error = ", err);
    return err;
  }
  return mem_info.RegionSize;
}

/**
 * \brief Unmaps a mapped view of a file from the calling process's address space.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile
 */
inline result<void> close_file_mapping(HANDLE h, LPCVOID mem) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  if (mem == NULL) {
    log.error("memory pointer is null.");
    return std::make_error_code(std::errc::invalid_argument);
  }
  if (!::UnmapViewOfFile(mem)) {
    log.warning("failed: UnmapViewOfFile. error = ", sys::error());
  }
  return winapi::close_handle(h);
}

enum class wait_result {
  object_0,
  abandoned,
  timeout
};

LIBIMP_INLINE_CONSTEXPR std::int64_t const infinite_time = -1;

/**
 * \brief Waits until the specified object is in the signaled state or the time-out interval elapses.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
 */
inline result<wait_result> wait_for_single_object(HANDLE h, std::int64_t ms) noexcept {
  LIBIMP_LOG_();
  DWORD dwMilliseconds = (ms < 0) ? INFINITE : static_cast<DWORD>(ms);
  DWORD r = ::WaitForSingleObject(h, dwMilliseconds);
  if (r == WAIT_FAILED) {
    auto err = sys::error();
    log.error("failed: WaitForSingleObject(", h, ", ", dwMilliseconds, "). error = ", err);
    return err;
  }
  if (r == WAIT_OBJECT_0) {
    return wait_result::object_0;
  }
  if (r == WAIT_ABANDONED) {
    return wait_result::abandoned;
  }
  return wait_result::timeout;
}

/**
 * \brief Waits until one or all of the specified objects are in the signaled state or the time-out interval elapses.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects
 */
inline result<wait_result> wait_for_multiple_objects(span<HANDLE const> handles, std::int64_t ms) noexcept {
  LIBIMP_LOG_();
  DWORD dwMilliseconds = (ms < 0) ? INFINITE : static_cast<DWORD>(ms);
  DWORD r = ::WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, dwMilliseconds);
  if (r == WAIT_FAILED) {
    auto err = sys::error();
    log.error("failed: WaitForMultipleObjects(", handles.size(), ", ", dwMilliseconds, "). error = ", err);
    return err;
  }
  if ((r >= WAIT_OBJECT_0) && (r < WAIT_OBJECT_0 + handles.size())) {
    return wait_result::object_0;
  }
  if ((r >= WAIT_ABANDONED_0) && (r < WAIT_ABANDONED_0 + handles.size())) {
    return wait_result::abandoned;
  }
  return wait_result::timeout;
}

/**
 * \brief Retrieves the current value of the performance counter, 
 *        which is a high resolution (<1us) time stamp that can be used for time-interval measurements.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
 */
inline result<std::int64_t> query_performance_counter() noexcept {
  LIBIMP_LOG_();
  std::int64_t pc;
  BOOL r = ::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&pc));
  if (!r) {
    auto err = sys::error();
    log.error("failed: QueryPerformanceCounter(). error = ", err);
    return err;
  }
  return pc;
}

/**
 * \brief Creates or opens a named or unnamed mutex object.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexa
 * \return Mutex object HANDLE, NULL on error.
 */
inline result<HANDLE> open_mutex(char const *name, bool initial_owner) noexcept {
  LIBIMP_LOG_();
  HANDLE h = ::CreateMutexA(winapi::get_security_descriptor(), initial_owner, name);
  if (h == NULL) {
    auto err = sys::error();
    log.error("failed: CreateMutexA(", initial_owner, ", ", name, "). error = ", err);
    return err;
  }
  return h;
}

/**
 * \brief Releases ownership of the specified mutex object.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasemutex
*/
inline result<bool> release_mutex(HANDLE h) noexcept {
  LIBIMP_LOG_();
  if (::ReleaseMutex(h)) {
    return true;
  }
  auto err = sys::error();
  log.error("failed: ReleaseMutex. error = ", err);
  return err;
}

/**
 * \brief Locks the mutex, blocks if the mutex is not available.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
*/
inline result<bool> wait_mutex(HANDLE h, std::int64_t ms) noexcept {
  LIBIMP_LOG_();
  // If the mutex is abandoned, we need to release it and try again.
  for (;;) {
    auto r = winapi::wait_for_single_object(h, ms);
    if (!r) {
      return r.error();
    }
    if (*r == winapi::wait_result::object_0) {
      return true;
    }
    if (*r == winapi::wait_result::abandoned) {
      log.info("failed: WaitForSingleObject(", ms, "). The mutex is abandoned, try again.");
      auto rr = release_mutex(h);
      if (rr) {
        continue;
      }
      return rr.error();
    }
    return false;
  }
}

} // namespace winapi
LIBIPC_NAMESPACE_END_

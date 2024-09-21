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
 * \brief Create a SECURITY_ATTRIBUTES structure singleton
 * \see https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa379560(v=vs.85)
 */
inline LPSECURITY_ATTRIBUTES get_sa() {
  static struct initiator {

    SECURITY_DESCRIPTOR sd_;
    SECURITY_ATTRIBUTES sa_;

    bool succ_ = false;

    initiator() {
      using namespace ::LIBIMP;
      LIBIMP_LOG_("get_sa");
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
result<HANDLE> mmap_open(std::string const &file, std::size_t size, mode::type type) noexcept {
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
    HANDLE h = ::CreateFileMapping(INVALID_HANDLE_VALUE, winapi::get_sa(), PAGE_READWRITE | SEC_COMMIT,
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
result<LPVOID> mmap_memof(HANDLE h) {
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
result<SIZE_T> mmap_sizeof(LPCVOID mem) {
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
result<void> mmap_release(HANDLE h, LPCVOID mem) {
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

} // namespace winapi
LIBIPC_NAMESPACE_END_

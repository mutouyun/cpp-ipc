/**
 * @file libipc/platform/win/mmap_impl.h
 * @author mutouyun (orz@orzz.org)
 */
#pragma once

#include <string>
#include <cstddef>

#include <Windows.h>

#include "libimp/log.h"
#include "libimp/system.h"

#include "libipc/shm.h"

#include "get_sa.h"
#include "to_tchar.h"

LIBIPC_NAMESPACE_BEG_
using namespace ::LIBIMP;

struct shm_handle {
  std::string file;
  std::size_t f_sz;
  void *memp;
  HANDLE h_fmap;
};

namespace {

/**
 * @brief Closes an open object handle.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
 */
result_code mmap_close(HANDLE h) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return {};
  }
  if (!::CloseHandle(h)) {
    auto err = sys::error();
    log.error("failed: CloseHandle(", h, "). error = ", err);
    return err.code();
  }
  return {ERROR_SUCCESS};
}

/**
 * @brief Creates or opens a file mapping object for a specified file.
 * 
 * @see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilemappinga
 *      https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga
 * 
 * @param file Specifies the name of the file mapping object
 * @param size Specifies the size required to create a file mapping object.
 *             This size is ignored when opening an existing file mapping object
 * @param type Combinable open modes, create | open
 * 
 * @return File mapping object HANDLE, NULL on error
 */
result<HANDLE> mmap_open(std::string const &file, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  if (file.empty()) {
    log.error("file name is empty.");
    return {};
  }
  auto t_name = detail::to_tstring(file);
  if (t_name.empty()) {
    log.error("file name is empty. (TCHAR conversion failed)");
    return {};
  }

  /// @brief Opens a named file mapping object.
  auto try_open = [&]() -> result<HANDLE> {
    HANDLE h = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, t_name.c_str());
    if (h == NULL) {
      auto err = sys::error();
      log.error("failed: OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, ", file, "). error = ", err);
      return {nullptr, err.value()};
    }
    return h;
  };

  /// @brief Creates or opens a named or unnamed file mapping object for a specified file.
  auto try_create = [&]() -> result<HANDLE> {
    HANDLE h = ::CreateFileMapping(INVALID_HANDLE_VALUE, detail::get_sa(), PAGE_READWRITE | SEC_COMMIT,
                                   /// @remark dwMaximumSizeHigh always 0 here.
                                   0, static_cast<DWORD>(size), t_name.c_str());
    if (h == NULL) {
      auto err = sys::error();
      log.error("failed: CreateFileMapping(PAGE_READWRITE | SEC_COMMIT, ", size, ", ", file, "). error = ", err);
      return {nullptr, err.value()};
    }
    return h;
  };

  if (type == mode::open) {
    return try_open();
  } else if ((type == (mode::create | mode::open)) && (size == 0)) {
    /// @remark CreateFileMapping may returns ERROR_INVALID_PARAMETER when dwMaximumSizeLow is zero.
    /// @see CreateFileMapping (Windows CE 5.0)
    ///      https://learn.microsoft.com/en-us/previous-versions/windows/embedded/aa517331(v=msdn.10)
    return try_open();
  } else if (!(type & mode::create)) {
    log.error("mode type is invalid. type = ", type);
    return {};
  }
  auto h = try_create();
  if (!h) return h;
  /// @remark If the object exists before the function call, the function returns a handle to the existing object
  ///         (with its current size, not the specified size), and GetLastError returns ERROR_ALREADY_EXISTS.
  if ((type == mode::create) && (::GetLastError() == ERROR_ALREADY_EXISTS)) {
    log.info("the file being created already exists. file = ", file, ", type = ", type);
    mmap_close(*h);
    return {};
  }
  return h;
}

/**
 * @brief Maps a view of a file mapping into the address space of a calling process.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile
 */
result<LPVOID> mmap_memof(HANDLE h) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return {};
  }
  LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (mem == NULL) {
    auto err = sys::error();
    log.error("failed: MapViewOfFile(", h, ", FILE_MAP_ALL_ACCESS). error = ", err);
    return {nullptr, err.value()};
  }
  return mem;
}

/**
 * @brief Retrieves the size about a range of pages in the virtual address space of the calling process.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualquery
 */
result<SIZE_T> mmap_sizeof(LPCVOID mem) {
  LIBIMP_LOG_();
  if (mem == NULL) {
    log.error("memory pointer is null.");
    return {};
  }
  MEMORY_BASIC_INFORMATION mem_info {};
  if (::VirtualQuery(mem, &mem_info, sizeof(mem_info)) == 0) {
    auto err = sys::error();
    log.error("failed: VirtualQuery(", mem, "). error = ", err);
    return {false, (SIZE_T)err.value()};
  }
  return mem_info.RegionSize;
}

/**
 * @brief Unmaps a mapped view of a file from the calling process's address space.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile
 */
result_code mmap_release(HANDLE h, LPCVOID mem) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return {};
  }
  if (mem == NULL) {
    log.error("memory pointer is null.");
    return {};
  }
  if (!::UnmapViewOfFile(mem)) {
    log.warning("failed: UnmapViewOfFile. error = ", sys::error());
  }
  return mmap_close(h);
}

} // namespace
LIBIPC_NAMESPACE_END_

/**
 * @file libipc/platform/win/mmap.h
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
using namespace ::LIBIMP_;

struct shm_handle {
  std::string file;
  std::size_t f_sz;
  void *memp;
  HANDLE h_fmap;
};

namespace {

void mmap_close(HANDLE h) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return;
  }
  /// @brief Closes an open object handle.
  /// @see https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
  if (!::CloseHandle(h)) {
    log.error("CloseHandle fails. error = {}", sys::error_msg(sys::error_code()));
  }
}

HANDLE mmap_open(std::string file, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  if (file.empty()) {
    log.error("file name is empty.");
    return NULL;
  }
  auto t_name = detail::to_tstring(file);
  if (t_name.empty()) {
    log.error("file name is empty. (TCHAR conversion failed)");
    return NULL;
  }
  /// @brief Opens a named file mapping object.
  /// @see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilemappinga
  if (type == mode::open) {
    HANDLE h = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, t_name.c_str());
    if (h == NULL) {
      log.error("OpenFileMapping fails. error = {}", sys::error_msg(sys::error_code()));
    }
    return h;
  }
  /// @brief Creates or opens a named or unnamed file mapping object for a specified file.
  /// @see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga
  HANDLE h = ::CreateFileMapping(INVALID_HANDLE_VALUE, detail::get_sa(), PAGE_READWRITE | SEC_COMMIT,
                                 0, static_cast<DWORD>(size), t_name.c_str());
  if (h == NULL) {
    log.error("CreateFileMapping fails. error = {}", sys::error_msg(sys::error_code()));
    return NULL;
  }
  // If the object exists before the function call, the function returns a handle to the existing object
  // (with its current size, not the specified size), and GetLastError returns ERROR_ALREADY_EXISTS.
  if ((type == mode::create) && (::GetLastError() == ERROR_ALREADY_EXISTS)) {
    mmap_close(h);
    return NULL;
  }
  return h;
}

LPVOID mmap_memof(HANDLE h) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return NULL;
  }
  /// @brief Maps a view of a file mapping into the address space of a calling process.
  /// @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile
  LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (h == NULL) {
    log.error("MapViewOfFile fails. error = {}", sys::error_msg(sys::error_code()));
    return NULL;
  }
  return mem;
}

SIZE_T mmap_sizeof(LPCVOID mem) {
  LIBIMP_LOG_();
  if (mem == NULL) {
    log.error("memory pointer is null.");
    return 0;
  }
  /// @brief Retrieves information about a range of pages in the virtual address space of the calling process.
  /// @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualquery
  MEMORY_BASIC_INFORMATION mem_info {};
  if (::VirtualQuery(mem, &mem_info, sizeof(mem_info)) == 0) {
    log.error("VirtualQuery fails. error = {}", sys::error_msg(sys::error_code()));
    return 0;
  }
  return mem_info.RegionSize;
}

void mmap_release(HANDLE h, LPCVOID mem) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return;
  }
  if (mem == NULL) {
    log.error("memory pointer is null.");
    return;
  }
  /// @brief Unmaps a mapped view of a file from the calling process's address space.
  /// @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile
  if (!::UnmapViewOfFile(mem)) {
    log.warning("UnmapViewOfFile fails. error = {}", sys::error_msg(sys::error_code()));
  }
  mmap_close(h);
}

} // namespace

::LIBIMP_::result<shm_t> shm_open(std::string name, std::size_t size, mode::type type) noexcept {
  return {};
}

LIBIPC_NAMESPACE_END_

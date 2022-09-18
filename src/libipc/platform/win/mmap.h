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

/**
 * @brief Closes an open object handle.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
 */
void mmap_close(HANDLE h) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return;
  }
  if (!::CloseHandle(h)) {
    log.error("CloseHandle fails. error = {}", sys::error_msg(sys::error_code()));
  }
}

/**
 * @brief Creates or opens a file mapping object for a specified file.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openfilemappinga
 *      https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga
 * @param file Specifies the name of the file mapping object
 * @param size Specifies the size required to create a file mapping object.
 *             This size is ignored when opening an existing file mapping object
 * @param type Combinable open modes, create | open
 * @return File mapping object HANDLE, NULL on error
 */
HANDLE mmap_open(std::string const &file, std::size_t size, mode::type type) noexcept {
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
  if (type == mode::open) {
    HANDLE h = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, t_name.c_str());
    if (h == NULL) {
      log.error("OpenFileMapping fails. error = {}", sys::error_msg(sys::error_code()));
    }
    return h;
  } else if (!(type & mode::create)) {
    log.error("mode type is invalid. type = {}", type);
    return NULL;
  }
  /// @brief Creates or opens a named or unnamed file mapping object for a specified file.
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
    log.info("the file being created already exists. file = {}, type = {}", file, type);
    return NULL;
  }
  return h;
}

/**
 * @brief Maps a view of a file mapping into the address space of a calling process.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile
 */
LPVOID mmap_memof(HANDLE h) {
  LIBIMP_LOG_();
  if (h == NULL) {
    log.error("handle is null.");
    return NULL;
  }
  LPVOID mem = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (h == NULL) {
    log.error("MapViewOfFile fails. error = {}", sys::error_msg(sys::error_code()));
    return NULL;
  }
  return mem;
}

/**
 * @brief Retrieves the size about a range of pages in the virtual address space of the calling process.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualquery
 */
SIZE_T mmap_sizeof(LPCVOID mem) {
  LIBIMP_LOG_();
  if (mem == NULL) {
    log.error("memory pointer is null.");
    return 0;
  }
  MEMORY_BASIC_INFORMATION mem_info {};
  if (::VirtualQuery(mem, &mem_info, sizeof(mem_info)) == 0) {
    log.error("VirtualQuery fails. error = {}", sys::error_msg(sys::error_code()));
    return 0;
  }
  return mem_info.RegionSize;
}

/**
 * @brief Unmaps a mapped view of a file from the calling process's address space.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile
 */
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
  if (!::UnmapViewOfFile(mem)) {
    log.warning("UnmapViewOfFile fails. error = {}", sys::error_msg(sys::error_code()));
  }
  mmap_close(h);
}

} // namespace

::LIBIMP_::result<shm_t> shm_open(std::string name, std::size_t size, mode::type type) noexcept {
  LIBIMP_LOG_();
  auto h = mmap_open(name, size, type);
  if (h == NULL) {
    log.error("mmap_open failed.");
    return {nullptr, *sys::error_code()};
  }
  auto mem = mmap_memof(h);
  if (mem == NULL) {
    log.warning("mmap_memof failed.");
  }
  return new shm_handle{std::move(name), mmap_sizeof(mem), mem, h};
}

::LIBIMP_::result_code shm_close(shm_t h) noexcept {
  LIBIMP_LOG_();
  if (h == nullptr) {
    log.error("shm handle is null.");
    return {};
  }
  auto shm = static_cast<shm_handle *>(h);
  mmap_release(shm->h_fmap, shm->memp);
  delete shm;
  return {true};
}

LIBIPC_NAMESPACE_END_

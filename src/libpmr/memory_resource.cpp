
#include <cstdlib>    // std::aligned_alloc
#include <cstddef>    // std::max_align_t
#include <exception>

#include "libimp/detect_plat.h"
#include "libimp/system.h"
#include "libimp/log.h"

#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_
namespace {

/**
 * @brief Check that bytes is not 0 and that the alignment is a power of two.
 */
bool verify_args(::LIBIMP_::log::gripper &log, std::size_t bytes, std::size_t alignment) noexcept {
  if (bytes == 0) {
    return false;
  }
  if ((alignment == 0) || (alignment & (alignment - 1)) != 0) {
    log.error("invalid bytes = {}, alignment = {}", bytes, alignment);
    return false;
  }
  return true;
}

} // namespace

/**
 * @brief Allocates storage with a size of at least bytes bytes, aligned to the specified alignment.
 * @remark Alignment shall be a power of two.
 * 
 * @see https://en.cppreference.com/w/cpp/memory/memory_resource/do_allocate
 *      https://www.cppstories.com/2019/08/newnew-align/
 * 
 * @return void * - nullptr if storage of the requested size and alignment cannot be obtained.
 */
void *new_delete_resource::do_allocate(std::size_t bytes, std::size_t alignment) noexcept {
  LIBIMP_LOG_();
  if (!verify_args(log, bytes, alignment)) {
    return nullptr;
  }
  if (alignment <= alignof(std::max_align_t)) {
    /// @see https://en.cppreference.com/w/cpp/memory/c/malloc
    return std::malloc(bytes);
  }
#if defined(LIBIMP_CPP_17)
  LIBIMP_TRY {
    /// @see https://en.cppreference.com/w/cpp/memory/c/aligned_alloc
    return std::aligned_alloc(alignment, bytes);
  } LIBIMP_CATCH(std::exception const &e) {
    log.error("std::aligned_alloc(alignment = {}, bytes = {}) fails. error = {}", 
               alignment, bytes, e.what());
    return nullptr;
  } LIBIMP_CATCH(...) {
    log.error("std::aligned_alloc(alignment = {}, bytes = {}) fails. error = unknown exception", 
               alignment, bytes);
    return nullptr;
  }
#elif defined(LIBIMP_OS_WIN)
  /// @see https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc
  return ::_aligned_malloc(bytes, alignment);
#else // try posix
  /// @see https://pubs.opengroup.org/onlinepubs/9699919799/functions/posix_memalign.html
  void *p = nullptr;
  int ret = ::posix_memalign(&p, alignment, bytes);
  if (ret != 0) {
    log.error("posix_memalign(alignment = {}, bytes = {}) fails. error = {}", 
               alignment, bytes, sys::error(ret));
    return nullptr;
  }
  return p;
#endif
}

/**
 * @brief Deallocates the storage pointed to by p.
 * @remark The storage it points to must not yet have been deallocated, otherwise the behavior is undefined.
 * 
 * @see https://en.cppreference.com/w/cpp/memory/memory_resource/do_deallocate
 * 
 * @param p must have been returned by a prior call to new_delete_resource::do_allocate(bytes, alignment).
 */
void new_delete_resource::do_deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
  LIBIMP_LOG_();
  if (p == nullptr) {
    return;
  }
  if (!verify_args(log, bytes, alignment)) {
    return;
  }
  if (alignment <= alignof(std::max_align_t)) {
    /// @see https://en.cppreference.com/w/cpp/memory/c/free
    std::free(p);
    return;
  }
#if defined(LIBIMP_CPP_17)
  LIBIMP_TRY {
    std::free(p);
  } LIBIMP_CATCH(std::exception const &e) {
    log.error("std::free(p = {}) fails, alignment = {}, bytes = {}. error = {}", 
               p, alignment, bytes, e.what());
  } LIBIMP_CATCH(...) {
    log.error("std::free(p = {}) fails, alignment = {}, bytes = {}. error = unknown exception", 
               p, alignment, bytes);
  }
#elif defined(LIBIMP_OS_WIN)
  /// @see https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-free
  ::_aligned_free(p);
#else // try posix
  ::free(p);
#endif
}

LIBPMR_NAMESPACE_END_

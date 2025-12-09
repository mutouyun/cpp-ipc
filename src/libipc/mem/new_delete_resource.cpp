
#include <cstdlib>  // std::aligned_alloc

#include "libipc/imp/detect_plat.h"
#include "libipc/imp/aligned.h"
#include "libipc/imp/system.h"
#include "libipc/imp/log.h"
#include "libipc/mem/memory_resource.h"
#include "libipc/mem/verify_args.h"

namespace ipc {
namespace mem {

/**
 * \brief Returns a pointer to a new_delete_resource.
 * 
 * \return new_delete_resource* 
 */
new_delete_resource *new_delete_resource::get() noexcept {
  static new_delete_resource mem_res;
  return &mem_res;
}

/**
 * \brief Allocates storage with a size of at least bytes bytes, aligned to the specified alignment.
 * Alignment shall be a power of two.
 * 
 * \see https://en.cppreference.com/w/cpp/memory/memory_resource/do_allocate
 *      https://www.cppstories.com/2019/08/newnew-align/
 * 
 * \return void * - nullptr if storage of the requested size and alignment cannot be obtained.
 */
void *new_delete_resource::allocate(std::size_t bytes, std::size_t alignment) noexcept {
  LIBIPC_LOG();
  if (!verify_args(bytes, alignment)) {
    log.error("invalid bytes = ", bytes, ", alignment = ", alignment);
    return nullptr;
  }
#if defined(LIBIPC_CPP_17)
  /// \see https://en.cppreference.com/w/cpp/memory/c/aligned_alloc
  /// \remark The size parameter must be an integral multiple of alignment.
  return std::aligned_alloc(alignment, ipc::round_up(bytes, alignment));
#else
  if (alignment <= alignof(std::max_align_t)) {
    /// \see https://en.cppreference.com/w/cpp/memory/c/malloc
    return std::malloc(bytes);
  }
#if defined(LIBIPC_OS_WIN)
  /// \see https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc
  return ::_aligned_malloc(bytes, alignment);
#else // try posix
  /// \see https://pubs.opengroup.org/onlinepubs/9699919799/functions/posix_memalign.html
  void *p = nullptr;
  int ret = ::posix_memalign(&p, alignment, bytes);
  if (ret != 0) {
    log.error("failed: posix_memalign(alignment = ", alignment, 
                                       ", bytes = ", bytes, 
                                      "). error = ", sys::error(ret));
    return nullptr;
  }
  return p;
#endif
#endif // defined(LIBIPC_CPP_17)
}

/**
 * \brief Deallocates the storage pointed to by p.
 * The storage it points to must not yet have been deallocated, otherwise the behavior is undefined.
 * 
 * \see https://en.cppreference.com/w/cpp/memory/memory_resource/do_deallocate
 * 
 * \param p must have been returned by a prior call to new_delete_resource::do_allocate(bytes, alignment).
 */
void new_delete_resource::deallocate(void *p, std::size_t bytes, std::size_t alignment) noexcept {
  LIBIPC_LOG();
  if (p == nullptr) {
    return;
  }
  if (!verify_args(bytes, alignment)) {
    log.error("invalid bytes = ", bytes, ", alignment = ", alignment);
    return;
  }
#if defined(LIBIPC_CPP_17)
  /// \see https://en.cppreference.com/w/cpp/memory/c/free
  std::free(p);
#else
  if (alignment <= alignof(std::max_align_t)) {
    std::free(p);
    return;
  }
#if defined(LIBIPC_OS_WIN)
  /// \see https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-free
  ::_aligned_free(p);
#else // try posix
  ::free(p);
#endif
#endif // defined(LIBIPC_CPP_17)
}

} // namespace mem
} // namespace ipc

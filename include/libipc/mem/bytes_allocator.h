/**
 * \file libipc/bytes_allocator.h
 * \author mutouyun (orz@orzz.org)
 * \brief A generic polymorphic memory allocator.
 */
#pragma once

#include <type_traits>
#include <array>
#include <limits>   // std::numeric_limits
#include <utility>  // std::forward
#include <tuple>    // std::ignore
#include <cstddef>

#include "libipc/imp/export.h"
#include "libipc/imp/uninitialized.h"
#include "libipc/imp/byte.h"

namespace ipc {
namespace mem {

/// \brief Helper trait for memory resource.

template <typename T, typename = void>
struct has_allocate : std::false_type {};

template <typename T>
struct has_allocate<T, 
  typename std::enable_if<std::is_convertible<
  decltype(std::declval<T &>().allocate(std::declval<std::size_t>(), 
                                        std::declval<std::size_t>())), void *
  >::value>::type> : std::true_type {};

template <typename T, typename = void>
struct has_deallocate : std::false_type {};

template <typename T>
struct has_deallocate<T, 
  decltype(std::declval<T &>().deallocate(std::declval<void *>(), 
                                          std::declval<std::size_t>(), 
                                          std::declval<std::size_t>()))
  > : std::true_type {};

template <typename T>
using is_memory_resource = 
  std::enable_if_t<has_allocate  <T>::value && 
                   has_deallocate<T>::value, bool>;

/**
 * \brief An allocator which exhibits different allocation behavior 
 *        depending upon the memory resource from which it is constructed.
 * 
 * \note Unlike `std::pmr::container_allocator`, it does not 
 *       rely on a specific inheritance relationship and only restricts 
 *       the interface behavior of the incoming memory resource object to 
 *       conform to `std::pmr::memory_resource`.
 * 
 * \see https://en.cppreference.com/w/cpp/memory/memory_resource
 *      https://en.cppreference.com/w/cpp/memory/container_allocator
 */
class LIBIPC_EXPORT bytes_allocator {

  class holder_mr_base {
  public:
    virtual ~holder_mr_base() noexcept = default;
    virtual void *alloc(std::size_t, std::size_t) const = 0;
    virtual void  dealloc(void *, std::size_t, std::size_t) const = 0;
  };

  template <typename MR, typename = bool>
  class holder_mr;

  /**
   * \brief An empty holding class used to calculate a reasonable memory size for the holder.
   * \tparam MR cannot be converted to the type of memory resource
   */
  template <typename MR, typename U>
  class holder_mr : public holder_mr_base {
  protected:
    MR *res_;

  public:
    holder_mr(MR *p_mr) noexcept
      : res_(p_mr) {}

    // [MSVC] error C2259: 'bytes_allocator::holder_mr<void *,bool>': cannot instantiate abstract class.
    void *alloc(std::size_t s, std::size_t a) const override { return nullptr; }
    void dealloc(void *p, std::size_t s, std::size_t a) const override {}
  };

  /**
   * \brief A memory resource pointer holder class for type erasure.
   * \tparam MR memory resource type
   */
  template <typename MR>
  class holder_mr<MR, is_memory_resource<MR>> : public holder_mr<MR, void> {
    using base_t = holder_mr<MR, void>;

  public:
    holder_mr(MR *p_mr) noexcept
      : base_t{p_mr} {}

    void *alloc(std::size_t s, std::size_t a) const override {
      return base_t::res_->allocate(s, a);
    }

    void dealloc(void *p, std::size_t s, std::size_t a) const override {
      base_t::res_->deallocate(p, s, a);
    }
  };

  using void_holder_t = holder_mr<void *>;
  alignas(void_holder_t) std::array<ipc::byte, sizeof(void_holder_t)> holder_;

  holder_mr_base &      get_holder() noexcept;
  holder_mr_base const &get_holder() const noexcept;

  void init_default_resource() noexcept;

public:
  /// \brief Constructs an `bytes_allocator` using the return value of 
  ///       `new_delete_resource::get()` as the underlying memory resource.
  bytes_allocator() noexcept;
  ~bytes_allocator() noexcept;

  bytes_allocator(bytes_allocator const &other) noexcept = default;
  bytes_allocator &operator=(bytes_allocator const &other) & noexcept = default;

  bytes_allocator(bytes_allocator &&other) noexcept = default;
  bytes_allocator &operator=(bytes_allocator &&other) & noexcept = default;

  /// \brief Constructs a `bytes_allocator` from a memory resource pointer.
  /// \note The lifetime of the pointer must be longer than that of bytes_allocator.
  template <typename T, is_memory_resource<T> = true>
  bytes_allocator(T *p_mr) noexcept {
    if (p_mr == nullptr) {
      init_default_resource();
      return;
    }
    std::ignore = ipc::construct<holder_mr<T>>(holder_.data(), p_mr);
  }

  void swap(bytes_allocator &other) noexcept;

  /// \brief Allocate/deallocate memory.
  void *allocate(std::size_t s, std::size_t = alignof(std::max_align_t)) const;
  void  deallocate(void *p, std::size_t s, std::size_t = alignof(std::max_align_t)) const;

  /// \brief Allocates uninitialized memory and constructs an object of type T in the memory.
  template <typename T, typename... A>
  T *construct(A &&...args) const {
    return ipc::construct<T>(allocate(sizeof(T), alignof(T)), std::forward<A>(args)...);
  }

  /// \brief Calls the destructor of the object pointed to by p and deallocates the memory.
  template <typename T>
  void destroy(T *p) const noexcept {
    deallocate(ipc::destroy(p), sizeof(T), alignof(T));
  }
};

} // namespace mem
} // namespace ipc

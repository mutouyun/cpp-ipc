/**
 * \file libpmr/allocator.h
 * \author mutouyun (orz@orzz.org)
 * \brief A generic polymorphic memory allocator.
 * \date 2022-11-13
 */
#pragma once

#include <type_traits>
#include <array>

#include "libimp/export.h"
#include "libimp/uninitialized.h"
#include "libimp/byte.h"

#include "libpmr/def.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_

/**
 * \brief An allocator which exhibits different allocation behavior 
 * depending upon the memory resource from which it is constructed.
 * 
 * \remarks Unlike std::pmr::polymorphic_allocator, it does not 
 * rely on a specific inheritance relationship and only restricts 
 * the interface behavior of the incoming memory resource object to 
 * conform to std::pmr::memory_resource.
 * 
 * \see https://en.cppreference.com/w/cpp/memory/memory_resource
 *      https://en.cppreference.com/w/cpp/memory/polymorphic_allocator
 */
class LIBIMP_EXPORT allocator {

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
  };

  /**
   * \brief A memory resource pointer holder class for type erasure.
   * \tparam MR memory resource type
   */
  template <typename MR>
  class holder_mr<MR, verify_memory_resource<MR>> : public holder_mr<MR, void> {
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
  alignas(void_holder_t) std::array<::LIBIMP::byte, sizeof(void_holder_t)> holder_;

  holder_mr_base &      get_holder() noexcept;
  holder_mr_base const &get_holder() const noexcept;

public:
  allocator() noexcept;
  ~allocator() noexcept;

  allocator(allocator const &other) noexcept = default;
  allocator &operator=(allocator const &other) & noexcept = default;

  allocator(allocator &&other) noexcept = default;
  allocator &operator=(allocator &&other) & noexcept = default;

  /// \brief Constructs a allocator from a memory resource pointer
  /// The lifetime of the pointer must be longer than that of allocator.
  template <typename T, verify_memory_resource<T> = true>
  allocator(T *p_mr) noexcept {
    if (p_mr == nullptr) {
      ::LIBIMP::construct<holder_mr<new_delete_resource>>(holder_.data(), new_delete_resource::get());
      return;
    }
    ::LIBIMP::construct<holder_mr<T>>(holder_.data(), p_mr);
  }

  void swap(allocator &other) noexcept;

  /// \brief Allocate/deallocate memory.
  void *allocate(std::size_t s, std::size_t = alignof(std::max_align_t)) const;
  void  deallocate(void *p, std::size_t s, std::size_t = alignof(std::max_align_t)) const;
};

LIBPMR_NAMESPACE_END_

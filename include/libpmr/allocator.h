/**
 * @file libpmr/allocator.h
 * @author mutouyun (orz@orzz.org)
 * @brief A generic polymorphic memory allocator.
 * @date 2022-11-13
 */
#pragma once

#include <type_traits>
#include <array>

#include "libimp/export.h"
#include "libimp/construct.h"
#include "libimp/byte.h"

#include "libpmr/def.h"
#include "libpmr/memory_resource.h"

LIBPMR_NAMESPACE_BEG_

/**
 * @brief An allocator which exhibits different allocation behavior 
 * depending upon the memory resource from which it is constructed.
 * 
 * @remarks Unlike std::pmr::polymorphic_allocator, it does not 
 * rely on a specific inheritance relationship and only restricts 
 * the interface behavior of the incoming memory resource object to 
 * conform to std::pmr::memory_resource.
 * 
 * @see https://en.cppreference.com/w/cpp/memory/memory_resource
 *      https://en.cppreference.com/w/cpp/memory/polymorphic_allocator
 */
class LIBIMP_EXPORT allocator {

  class holder_base {
  public:
    virtual ~holder_base() noexcept = default;
    virtual void *alloc(std::size_t) = 0;
    virtual void  free (void *, std::size_t) = 0;
    virtual bool  valid() const noexcept = 0;
  };

  class holder_null : public holder_base {
  public:
    void *alloc(std::size_t) override { return nullptr; }
    void  free (void *, std::size_t) override {}
    bool  valid() const noexcept override { return false; }
  };

  template <typename MemRes, typename = void>
  class holder_memory_resource;

  /**
   * @brief A memory resource pointer holder class for type erasure.
   * @tparam MR memory resource type
   */
  template <typename MR>
  class holder_memory_resource<MR, is_memory_resource<MR>> : public holder_base {
    MR *p_mem_res_;

  public:
    holder_memory_resource(MR *p_mr) noexcept
      : p_mem_res_(p_mr) {}

    void *alloc(std::size_t s) override {
      return p_mem_res_->allocate(s);
    }

    void free(void *p, std::size_t s) override {
      p_mem_res_->deallocate(p, s);
    }
  
    bool valid() const noexcept override {
      return p_mem_res_ != nullptr;
    }
  };

  /**
   * @brief An empty holding class used to calculate a reasonable memory size for the holder.
   * @tparam MR cannot be converted to the type of memory resource
   */
  template <typename MR, typename U>
  class holder_memory_resource : public holder_null {
    MR *p_dummy_;
  };

  using void_holder_type = holder_memory_resource<void>;
  alignas(void_holder_type) std::array<::LIBIMP::byte, sizeof(void_holder_type)> holder_;

  holder_base &      get_holder() noexcept;
  holder_base const &get_holder() const noexcept;

public:
  allocator() noexcept;
  ~allocator() noexcept;

  allocator(allocator const &other) noexcept = default;
  allocator &operator=(allocator const &other) & noexcept = default;

  allocator(allocator &&other) noexcept;
  allocator &operator=(allocator &&other) & noexcept;

  /// @brief Constructs a allocator from a memory resource pointer
  /// @remark The lifetime of the pointer must be longer than that of allocator.
  template <typename T, typename = is_memory_resource<T>>
  allocator(T *p_mr) : allocator() {
    if (p_mr == nullptr) return;
    ::LIBIMP::construct<holder_memory_resource<T>>(holder_.data(), p_mr);
  }

  void swap(allocator &other) noexcept;
  bool valid() const noexcept;
  explicit operator bool() const noexcept;

  /// @brief Allocate/deallocate memory.
  void *alloc(std::size_t s);
  void  free (void *p, std::size_t s);
};

LIBPMR_NAMESPACE_END_

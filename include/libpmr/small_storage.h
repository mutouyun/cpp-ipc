/**
 * \file libpmr/small_storage.h
 * \author mutouyun (orz@orzz.org)
 * \brief Unified SSO (Small Size Optimization).
 * \date 2023-09-02
 */
#pragma once

#include <type_traits>
#include <array>
#include <typeinfo>
#include <utility>
#include <memory>
#include <cstddef>

#include "libimp/export.h"
#include "libimp/uninitialized.h"
#include "libimp/byte.h"
#include "libimp/generic.h"

#include "libpmr/def.h"
#include "libpmr/allocator.h"

LIBPMR_NAMESPACE_BEG_

/**
 * \class holder_base
 * \brief Data holder base class.
*/
class holder_base {

  // non-copyable
  holder_base(holder_base const &) = delete;
  holder_base &operator=(holder_base const &) = delete;

public:
  holder_base() noexcept = default;
  virtual ~holder_base() noexcept = default;
  virtual bool valid() const noexcept = 0;
  virtual std::type_info const &type() const noexcept = 0;
  virtual std::size_t sizeof_type() const noexcept = 0;
  virtual std::size_t sizeof_heap() const noexcept = 0;
  virtual std::size_t count() const noexcept = 0;
  virtual void *get() noexcept = 0;
  virtual void const *get() const noexcept = 0;

  /// \brief The passed destination pointer is never null, 
  /// and the memory to which it points is uninitialized.
  virtual void move_to(allocator const &, void *) noexcept = 0;
  virtual void copy_to(allocator const &, void *) const noexcept(false) = 0;
  virtual void destroy(allocator const &) noexcept = 0;
};

/**
 * \class holder_null
 * \brief A holder implementation that does not hold any data objects.
*/
class holder_null : public holder_base {
public:
  bool valid() const noexcept override { return false; }
  std::type_info const &type() const noexcept override { return typeid(nullptr);}
  std::size_t sizeof_type() const noexcept override { return 0; }
  std::size_t sizeof_heap() const noexcept override { return 0; }
  std::size_t count() const noexcept override { return 0; }
  void *get() noexcept override { return nullptr; }
  void const *get() const noexcept override { return nullptr; }
  void move_to(allocator const &, void *) noexcept override {}
  void copy_to(allocator const &, void *) const noexcept(false) override {}
  void destroy(allocator const &) noexcept override {}
};

template <typename Value, bool/*is on stack*/>
class holder;

/**
 * \class template <typename Value> holder<Value, true>
 * \brief A holder implementation that holds a data object if type `Value` on stack memory.
 * \tparam Value The storage type of the holder.
 */
template <typename Value>
class holder<Value, true> : public holder_base {

  alignas(alignof(Value)) std::array<::LIBIMP::byte, sizeof(Value)> value_storage_;

public:
  holder() = default; // uninitialized

  template <typename... A>
  holder(allocator const &, A &&...args) {
    ::LIBIMP::construct<Value>(value_storage_.data(), std::forward<A>(args)...);
  }

  bool valid() const noexcept override {
    return true;
  }

  std::type_info const &type() const noexcept override {
    return typeid(Value);
  }

  std::size_t sizeof_type() const noexcept override {
    return sizeof(Value);
  }

  std::size_t sizeof_heap() const noexcept override {
    return 0;
  }

  std::size_t count() const noexcept override {
    return 1;
  }

  void *get() noexcept override {
    return value_storage_.data();
  }

  void const *get() const noexcept override {
    return value_storage_.data();
  }

  void move_to(allocator const &alloc, void *p) noexcept override {
    ::LIBIMP::construct<holder>(p, alloc, std::move(*static_cast<Value *>(get())));
  }

  void copy_to(allocator const &alloc, void *p) const noexcept(false) override {
    ::LIBIMP::construct<holder>(p, alloc, *static_cast<Value const *>(get()));
  }

  void destroy(allocator const &) noexcept override {
    ::LIBIMP::destroy(static_cast<Value *>(get()));
  }
};

/**
 * \class template <typename Value> holder<Value, false>
 * \brief A holder implementation that holds a data object if type `Value` on heap memory.
 * \tparam Value The storage type of the holder.
 */
template <typename Value>
class holder<Value, false> : public holder_base {

  Value *value_ptr_;

public:
  holder() noexcept : value_ptr_(nullptr) {}

  template <typename... A>
  holder(allocator const &alloc, A &&...args) {
    void *p = alloc.allocate(sizeof(Value), alignof(Value));
    if (p == nullptr) return;
    value_ptr_ = ::LIBIMP::construct<Value>(p, std::forward<A>(args)...);
  }

  bool valid() const noexcept override {
    return value_ptr_ != nullptr;
  }

  std::type_info const &type() const noexcept override {
    return typeid(Value);
  }

  std::size_t sizeof_type() const noexcept override {
    return sizeof(Value);
  }

  std::size_t sizeof_heap() const noexcept override {
    return sizeof(Value);
  }

  std::size_t count() const noexcept override {
    return 1;
  }

  void *get() noexcept override {
    return value_ptr_;
  }

  void const *get() const noexcept override {
    return value_ptr_;
  }

  void move_to(allocator const &alloc, void *p) noexcept override {
    if (value_ptr_ == nullptr) {
      ::LIBIMP::construct<holder>(p);
      return;
    }
    ::LIBIMP::construct<holder>(p, alloc, std::move(*value_ptr_));
  }

  void copy_to(allocator const &alloc, void *p) const noexcept(false) override {
    if (value_ptr_ == nullptr) {
      ::LIBIMP::construct<holder>(p);
      return;
    }
    ::LIBIMP::construct<holder>(p, alloc, *value_ptr_);
  }

  void destroy(allocator const &alloc) noexcept override {
    alloc.deallocate(::LIBIMP::destroy(value_ptr_), sizeof(Value), alignof(Value));
  }
};

namespace detail {

struct holder_info {
  std::type_info const *type;
  std::size_t sizeof_type;
  std::size_t count;
  void (*copy)(allocator const &, void const *s, void *d);
  void (*dest)(void *p, std::size_t n) noexcept;
};

template <typename Value, typename Construct>
void *holder_construct(allocator const &alloc, std::size_t n, Construct &&c) {
  void *p = alloc.allocate(n * sizeof(Value));
  if (p == nullptr) return nullptr;
  LIBIMP_TRY {
    std::forward<Construct>(c)(static_cast<Value *>(p), n);
    return p;
  } LIBIMP_CATCH(...) {
    alloc.deallocate(p, n * sizeof(Value));
    LIBIMP_THROW(, nullptr);
  }
}

} // namespace detail

/**
 * \class template <> holder<void, true>
 * \brief A holder implementation of some data objects that stores type information on stack memory.
 */
template <>
class holder<void, true> : public holder_base {

  detail::holder_info info_;
  void *value_ptr_;

public:
  holder() noexcept
    : info_{&typeid(nullptr)}
    , value_ptr_(nullptr) {}

  /// \brief Constructs a holder with type of `Value`.
  /// alignof(Value) must be less than or equal to alignof(std::max_align_t).
  template <typename Value, 
            std::enable_if_t<alignof(Value) <= alignof(std::max_align_t), bool> = true>
  holder(allocator const &alloc, ::LIBIMP::types<Value>, std::size_t n) : holder() {
    value_ptr_ = detail::holder_construct<Value>(alloc, n, 
      ::LIBIMP::uninitialized_default_construct_n<Value *, std::size_t>);
    if (value_ptr_ == nullptr) return;
    info_.type = &typeid(Value);
    info_.sizeof_type = sizeof(Value);
    info_.count = n;
    info_.copy = [](allocator const &alloc, void const *s, void *d) {
      auto const &src = *static_cast<holder const *>(s);
      auto &      dst = *::LIBIMP::construct<holder>(d);
      if (!src.valid()) return;
      dst.value_ptr_ = detail::holder_construct<Value>(alloc, src.count(), [s = src.get()](Value *d, std::size_t n) {
        std::uninitialized_copy_n(static_cast<Value const *>(s), n, d);
      });
      if (dst.value_ptr_ == nullptr) return;
      dst.info_ = src.info_;
    };
    info_.dest = [](void *p, std::size_t n) noexcept {
      ::LIBIMP::destroy_n(static_cast<Value *>(p), n);
    };
  }

  bool valid() const noexcept override {
    return value_ptr_ != nullptr;
  }

  std::type_info const &type() const noexcept override {
    return *info_.type;
  }

  std::size_t sizeof_type() const noexcept override {
    return info_.sizeof_type;
  }

  std::size_t sizeof_heap() const noexcept override {
    return info_.sizeof_type * info_.count;
  }

  std::size_t count() const noexcept override {
    return info_.count;
  }

  void *get() noexcept override {
    return value_ptr_;
  }

  void const *get() const noexcept override {
    return value_ptr_;
  }

  void move_to(allocator const &, void *p) noexcept override {
    auto *des = ::LIBIMP::construct<holder>(p);
    if (!valid()) return;
    std::swap(value_ptr_, des->value_ptr_);
    std::swap(info_, des->info_);
  }

  void copy_to(allocator const &alloc, void *p) const noexcept(false) override {
    auto *des = ::LIBIMP::construct<holder>(p);
    if (!valid()) return;
    info_.copy(alloc, this, des);
  }

  void destroy(allocator const &alloc) noexcept override {
    if (!valid()) return;
    info_.dest(value_ptr_, count());
    alloc.deallocate(value_ptr_, sizeof_heap());
  }
};

LIBPMR_NAMESPACE_END_

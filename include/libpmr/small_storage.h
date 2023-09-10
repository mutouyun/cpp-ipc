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
#include "libimp/aligned.h"

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

  ::LIBIMP::aligned<Value> value_storage_;

public:
  holder() = default; // uninitialized

  template <typename... A>
  holder(allocator const &, A &&...args) {
    ::LIBIMP::construct<Value>(value_storage_.ptr(), std::forward<A>(args)...);
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
    return value_storage_.ptr();
  }

  void const *get() const noexcept override {
    return value_storage_.ptr();
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
  void (*move)(void *s, void *d);
  void (*dest)(void *p, std::size_t n);
};

template <typename Value>
std::size_t full_sizeof(std::size_t count) noexcept {
  return ::LIBIMP::round_up(sizeof(detail::holder_info), alignof(std::max_align_t))
        + (sizeof(Value) * count);
}

std::size_t full_sizeof(holder_info const *info) noexcept {
  return ::LIBIMP::round_up(sizeof(detail::holder_info), alignof(std::max_align_t))
        + (info->sizeof_type * info->count);
}

void *value_ptr(holder_info *info) noexcept {
  return reinterpret_cast<void *>(
      ::LIBIMP::round_up(reinterpret_cast<std::size_t>(info + 1), alignof(std::max_align_t)));
}

void const *value_ptr(holder_info const *info) noexcept {
  return reinterpret_cast<void const *>(
      ::LIBIMP::round_up(reinterpret_cast<std::size_t>(info + 1), alignof(std::max_align_t)));
}

class holder_info_ptr {
  allocator const &alloc_;
  holder_info **pptr_;
  std::size_t size_;

public:
  holder_info_ptr(allocator const &alloc, holder_info *(&ptr), std::size_t sz) noexcept
    : alloc_(alloc)
    , pptr_ (&ptr)
    , size_ (::LIBIMP::round_up(sizeof(holder_info), alignof(std::max_align_t)) + sz) {
    *pptr_ = static_cast<holder_info *>(alloc_.allocate(size_));
  }

  ~holder_info_ptr() noexcept {
    if (pptr_ == nullptr) return;
    alloc_.deallocate(*pptr_, size_);
    *pptr_ = nullptr;
  }

  explicit operator bool() const noexcept {
    return (pptr_ != nullptr) && (*pptr_ != nullptr);
  }

  holder_info *operator->() const noexcept {
    return *pptr_;
  }

  holder_info &operator*() noexcept {
    return **pptr_;
  }

  void release() noexcept {
    pptr_ = nullptr;
  }
};

} // namespace detail

/**
 * \class template <> holder<void, true>
 * \brief A holder implementation of some data objects that stores type information on stack memory.
 */
template <>
class holder<void, true> : public holder_base {

  detail::holder_info info_;

public:
  template <typename Value>
  static std::size_t full_sizeof(std::size_t count) noexcept {
    return offsetof(holder, info_) + detail::full_sizeof<Value>(count);
  }

  holder() noexcept
    : info_{&typeid(nullptr)} {}

  /// \brief Constructs a holder with type of `Value`.
  /// alignof(Value) must be less than or equal to alignof(std::max_align_t).
  template <typename Value, 
            std::enable_if_t<alignof(Value) <= alignof(std::max_align_t), bool> = true>
  holder(allocator const &, ::LIBIMP::types<Value>, std::size_t n) : holder() {
    ::LIBIMP::uninitialized_default_construct_n(static_cast<Value *>(get()), n);
    info_.type = &typeid(Value);
    info_.sizeof_type = sizeof(Value);
    info_.count = n;
    info_.copy = [](allocator const &, void const *s, void *d) {
      auto const &src = *static_cast<holder const *>(s);
      auto &      dst = *static_cast<holder *      >(d);
      std::uninitialized_copy_n(static_cast<Value const *>(src.get()), src.count(), 
                                static_cast<Value *      >(dst.get()));
      dst.info_ = src.info_;
    };
    info_.move = [](void *s, void *d) noexcept {
      auto &src = *static_cast<holder *>(s);
      auto &dst = *static_cast<holder *>(d);
      std::uninitialized_move_n(static_cast<Value *>(src.get()), src.count(), 
                                static_cast<Value *>(dst.get()));
      std::swap(dst.info_, src.info_);
    };
    info_.dest = [](void *p, std::size_t n) noexcept {
      ::LIBIMP::destroy_n(static_cast<Value *>(p), n);
    };
  }

  bool valid() const noexcept override {
    return (info_.type != nullptr) && (info_.type != &typeid(nullptr));
  }

  std::type_info const &type() const noexcept override {
    if (!valid()) return typeid(nullptr);
    return *info_.type;
  }

  std::size_t sizeof_type() const noexcept override {
    return info_.sizeof_type;
  }

  std::size_t sizeof_heap() const noexcept override {
    return 0;
  }

  std::size_t count() const noexcept override {
    return info_.count;
  }

  void *get() noexcept override {
    return detail::value_ptr(&info_);
  }

  void const *get() const noexcept override {
    return detail::value_ptr(&info_);
  }

  void move_to(allocator const &, void *p) noexcept override {
    auto *des = ::LIBIMP::construct<holder>(p);
    if (!valid()) return;
    info_.move(this, des);
  }

  void copy_to(allocator const &alloc, void *p) const noexcept(false) override {
    auto *des = ::LIBIMP::construct<holder>(p);
    if (!valid()) return;
    info_.copy(alloc, this, des);
  }

  void destroy(allocator const &) noexcept override {
    if (!valid()) return;
    info_.dest(get(), count());
  }
};

/**
 * \class template <> holder<void, false>
 * \brief A holder implementation of some data objects that stores type information on heap memory.
 */
template <>
class holder<void, false> : public holder_base {

  detail::holder_info *info_ptr_;

public:
  holder() noexcept
    : info_ptr_(nullptr) {}

  /// \brief Constructs a holder with type of `Value`.
  /// alignof(Value) must be less than or equal to alignof(std::max_align_t).
  template <typename Value, 
            std::enable_if_t<alignof(Value) <= alignof(std::max_align_t), bool> = true>
  holder(allocator const &alloc, ::LIBIMP::types<Value>, std::size_t n) : holder() {
    detail::holder_info_ptr info_p{alloc, info_ptr_, sizeof(Value) * n};
    if (!info_p) return;
    ::LIBIMP::uninitialized_default_construct_n(static_cast<Value *>(get()), n);
    info_p->type = &typeid(Value);
    info_p->sizeof_type = sizeof(Value);
    info_p->count = n;
    info_p->copy = [](allocator const &alloc, void const *s, void *d) {
      auto const &src = *static_cast<holder const *>(s);
      auto &      dst = *static_cast<holder *      >(d);
      detail::holder_info_ptr info_p{alloc, dst.info_ptr_, sizeof(Value) * src.count()};
      if (!info_p) return;
      ::LIBIMP::uninitialized_default_construct_n(static_cast<Value *>(dst.get()), src.count());
      *info_p = *src.info_ptr_;
      info_p.release();
    };
    info_p->dest = [](void *p, std::size_t n) noexcept {
      ::LIBIMP::destroy_n(static_cast<Value *>(p), n);
    };
    info_p.release();
  }

  bool valid() const noexcept override {
    return info_ptr_ != nullptr;
  }

  std::type_info const &type() const noexcept override {
    if (!valid()) return typeid(nullptr);
    return *info_ptr_->type;
  }

  std::size_t sizeof_type() const noexcept override {
    if (!valid()) return 0;
    return info_ptr_->sizeof_type;
  }

  std::size_t sizeof_heap() const noexcept override {
    if (!valid()) return 0;
    return detail::full_sizeof(info_ptr_);
  }

  std::size_t count() const noexcept override {
    if (!valid()) return 0;
    return info_ptr_->count;
  }

  void *get() noexcept override {
    return detail::value_ptr(info_ptr_);
  }

  void const *get() const noexcept override {
    return detail::value_ptr(info_ptr_);
  }

  void move_to(allocator const &, void *p) noexcept override {
    auto *des = ::LIBIMP::construct<holder>(p);
    if (!valid()) return;
    std::swap(info_ptr_, des->info_ptr_);
  }

  void copy_to(allocator const &alloc, void *p) const noexcept(false) override {
    auto *des = ::LIBIMP::construct<holder>(p);
    if (!valid()) return;
    info_ptr_->copy(alloc, this, des);
  }

  void destroy(allocator const &alloc) noexcept override {
    if (!valid()) return;
    info_ptr_->dest(get(), count());
    alloc.deallocate(info_ptr_, sizeof_heap());
  }
};

/**
 * \class template <std::size_t N> small_storage
 * \brief Unified SSO (Small Size Optimization).
 * 
 * \note `small_storage` does not release resources at destructor time, 
 * the user needs to manually call the `release` function and 
 * specify the appropriate allocator to complete the resource release.
 * 
 * \tparam N The size of the storage.
 */
template <std::size_t N>
class small_storage {

  static_assert(N >= sizeof(holder<void *, false>), "N must be greater than sizeof(holder<void *, false>)");

  alignas(std::max_align_t) std::array<::LIBIMP::byte, N> storage_;

  /// \brief Try to construct an object on the stack, and if there is not enough space, 
  /// the memory allocator allocates heap memory to complete the construction.
  template <typename T>
  struct applicant {
    template <typename... A>
    T *operator()(small_storage *self, allocator const &alloc, A &&...args) const noexcept(false) {
      self->get_holder()->destroy(alloc);
      constexpr bool on_stack = (sizeof(holder<T, true>) <= N);
      ::LIBIMP::construct<holder<T, on_stack>>(self->get_holder(), alloc, std::forward<A>(args)...);
      return self->as<T>();
    }
  };

  /// \brief Try to construct an array of objects on the stack, and if there is not enough space, 
  /// the memory allocator allocates heap memory to complete the construction.
  template <typename T>
  struct applicant<T[]> {
    T *operator()(small_storage *self, allocator const &alloc, std::size_t n) const noexcept(false) {
      self->get_holder()->destroy(alloc);
      if (holder<void, true>::full_sizeof<T>(n) <= N) {
        ::LIBIMP::construct<holder<void, true>>(self->get_holder(), alloc, ::LIBIMP::types<T>{}, n);
      } else {
        ::LIBIMP::construct<holder<void, false>>(self->get_holder(), alloc, ::LIBIMP::types<T>{}, n);
      }
      return self->as<T>();
    }
  };

public:
  small_storage(small_storage const &) = delete;
  small_storage &operator=(small_storage const &) = delete;

  small_storage() noexcept {
    ::LIBIMP::construct<holder_null>(get_holder());
  }

  holder_base *get_holder() noexcept {
    return reinterpret_cast<holder_base *>(storage_.data());
  }

  holder_base const *get_holder() const noexcept {
    return reinterpret_cast<holder_base const *>(storage_.data());
  }

  bool valid() const noexcept {
    return get_holder()->valid();
  }

  std::type_info const &type() const noexcept {
    return get_holder()->type();
  }

  std::size_t sizeof_type() const noexcept {
    return get_holder()->sizeof_type();
  }

  std::size_t sizeof_heap() const noexcept {
    return get_holder()->sizeof_heap();
  }

  std::size_t count() const noexcept {
    return get_holder()->count();
  }

  template <typename Value>
  Value *as() noexcept {
    return static_cast<Value *>(get_holder()->get());
  }

  template <typename Value>
  Value const *as() const noexcept {
    return static_cast<Value const *>(get_holder()->get());
  }

  template <typename Value>
  Value &as_ref() noexcept {
    return *as<Value>();
  }

  template <typename Value>
  Value const &as_ref() const noexcept {
    return *as<Value>();
  }

  void move_to(allocator const &alloc, small_storage &des) noexcept {
    get_holder()->move_to(alloc, des.get_holder());
  }

  void copy_to(allocator const &alloc, small_storage &des) const noexcept(false) {
    get_holder()->copy_to(alloc, des.get_holder());
  }

  template <typename Value, typename... A>
  auto acquire(allocator const &alloc, A &&...args) noexcept(false) {
    return applicant<Value>{}(this, alloc, std::forward<A>(args)...);
  }

  void release(allocator const &alloc) noexcept {
    if (!valid()) return;
    get_holder()->destroy(alloc);
    ::LIBIMP::construct<holder_null>(get_holder());
  }
};

LIBPMR_NAMESPACE_END_

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
#include "libimp/construct.h"
#include "libimp/byte.h"
#include "libimp/generic.h"

#include "libpmr/def.h"

LIBPMR_NAMESPACE_BEG_

class LIBIMP_EXPORT allocator;

/**
 * \class holder_base
 * \brief Data holder base class.
*/
class holder_base {
public:
  virtual ~holder_base() noexcept = default;
  virtual bool valid() const noexcept = 0;
  virtual std::type_info const &type() const noexcept = 0;
  virtual std::size_t sizeof_type() const noexcept = 0;
  virtual std::size_t count() const noexcept = 0;
  virtual std::size_t size () const noexcept = 0;
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
  std::size_t count() const noexcept override { return 0; }
  std::size_t size () const noexcept override { return 0; }
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
  holder(::LIBIMP::in_place_t, A &&...args) {
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

  std::size_t count() const noexcept override {
    return 1;
  }

  std::size_t size() const noexcept override {
    return value_storage_.size();
  }

  void *get() noexcept override {
    return value_storage_.data();
  }

  void const *get() const noexcept override {
    return value_storage_.data();
  }

  void move_to(allocator const &, void *p) noexcept override {
    ::LIBIMP::construct<holder>(p, ::LIBIMP::in_place, std::move(*static_cast<Value *>(get())));
  }

  void copy_to(allocator const &, void *p) const noexcept(false) override {
    ::LIBIMP::construct<holder>(p, ::LIBIMP::in_place, *static_cast<Value *>(get()));
  }

  void destroy(allocator const &) noexcept override {
    ::LIBIMP::destroy<Value>(value_storage_.data());
  }
};

/**
 * \class template <typename Value> holder<Value, false>
 * \brief A holder implementation that holds a data object if type `Value` on heap memory.
 * \tparam Value The storage type of the holder.
 */
template <typename Value>
class holder<Value, false> : public holder_base {

  Value *value_ptr_ = nullptr;

public:
  holder() = default; // uninitialized
};

LIBPMR_NAMESPACE_END_

/**
 * \file libconcur/queue.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define concurrent queue.
 * \date 2022-11-19
 */
#pragma once

#include <cstddef>

#include "libimp/construct.h"
#include "libimp/detect_plat.h"

#include "libpmr/allocator.h"
#include "libpmr/memory_resource.h"

#include "libconcur/def.h"
#include "libconcur/concurrent.h"

LIBCONCUR_NAMESPACE_BEG_

template <typename T, typename PRelationT, typename CRelationT>
class queue {
public:
  using producer_relation_t = PRelationT;
  using consumer_relation_t = CRelationT;
  using model_type = prod_cons<trans::unicast, producer_relation_t, consumer_relation_t>;
  using value_type = T;

private:
  struct data {
    model_type model_;
    typename concur::traits<model_type>::header header_;
    element<value_type> elements_start_;

    template <typename U>
    data(U &&model) noexcept
      : header_(std::forward<U>(model)) {}

    /// \brief element<value_type> elements[0];
    ::LIBIMP::span<element<value_type>> elements() noexcept {
      return {&elements_start_, header_.circ_size};
    }
  };

  data *init(index_t circ_size) noexcept {
    if (!data_allocator_) {
      return nullptr;
    }
    LIBIMP_TRY {
      auto data_ptr = data_allocator_.allocate(sizeof(data) + (circ_size - 1) * sizeof(element<value_type>));
      if (data_ptr == nullptr) {
        return nullptr;
      }
      return ::LIBIMP::construct<data>(data_ptr, circ_size);
    } LIBIMP_CATCH(...) {
      return nullptr;
    }
  }

  ::LIBPMR::allocator data_allocator_;
  data *data_;
  typename concur::traits<model_type>::context context_;

public:
  queue() = default;
  queue(queue const &) = delete;
  queue(queue &&) = delete;
  queue &operator=(queue const &) = delete;
  queue &operator=(queue &&) = delete;

  template <typename MR, ::LIBPMR::verify_memory_resource<T> = true>
  explicit queue(index_t circ_size = default_circle_buffer_size, 
                 MR *memory_resource = ::LIBPMR::new_delete_resource::get()) noexcept
    : data_allocator_(memory_resource)
    , data_(init(circ_size)) {}

  template <typename MR, ::LIBPMR::verify_memory_resource<T> = true>
  explicit queue(MR *memory_resource) noexcept
    : queue(default_circle_buffer_size, memory_resource) {}

  bool valid() const noexcept {
    return data_ != nullptr;
  }

  explicit operator bool() const noexcept {
    return valid();
  }

  template <typename U>
  bool push(U &&value) noexcept {
    if (!valid()) return false;
    return data_->model_.enqueue(data_->elements(), data_->header_, context_, std::forward<U>(value));
  }

  bool pop(value_type &value) noexcept {
    if (!valid()) return false;
    return data_->model_.dequeue(data_->elements(), data_->header_, context_, value);
  }
};

LIBCONCUR_NAMESPACE_END_

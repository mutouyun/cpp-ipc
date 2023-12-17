/**
 * \file libconcur/queue.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define concurrent circular queue.
 * \date 2022-11-19
 */
#pragma once

#include "libconcur/def.h"
#include "libconcur/data_model.h"

LIBCONCUR_NAMESPACE_BEG_

template <typename T, typename PRelationT = relation::multi
                    , typename CRelationT = relation::multi>
class circular_queue
  : public data_model<T, trans::unicast, PRelationT, CRelationT> {

  using base_t = data_model<T, trans::unicast, PRelationT, CRelationT>;

public:
  using base_t::base_t;

  template <typename U>
  bool push(U &&value) noexcept {
    return this->enqueue(std::forward<U>(value));
  }

  bool pop(typename base_t::value_type &value) noexcept {
    return this->dequeue(value);
  }
};

LIBCONCUR_NAMESPACE_END_

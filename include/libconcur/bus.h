/**
 * \file libconcur/bus.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define concurrent data bus.
 * \date 2023-04-17
 */
#pragma once

#include "libconcur/def.h"
#include "libconcur/data_model.h"

LIBCONCUR_NAMESPACE_BEG_

template <typename T, typename PRelationT = relation::multi>
class bus
  : public data_model<T, trans::broadcast, PRelationT, relation::multi> {

  using base_t = data_model<T, trans::broadcast, PRelationT, relation::multi>;

public:
  using base_t::base_t;

  template <typename U>
  bool broadcast(U &&value) noexcept {
    return this->enqueue(std::forward<U>(value));
  }

  bool receive(typename base_t::value_type &value) noexcept {
    return this->dequeue(value);
  }
};

LIBCONCUR_NAMESPACE_END_

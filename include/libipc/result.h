/**
 * @file result.h
 * @author mutouyun (orz@orzz.org)
 * @brief Define the return value type with a status code
 * @date 2022-04-17
 */
#pragma once

#include <ostream>  // std::ostream
#include <cstdint>

#include "libipc/def.h"
#include "libipc/export.h"

LIBIPC_NAMESPACE_BEG_

class LIBIPC_EXPORT result {
  std::uint64_t status_;

public:
  result() noexcept;
  result(bool ok, std::uint64_t code = 0) noexcept;

  std::uint64_t code() const noexcept;
  bool ok() const noexcept;

  explicit operator bool() const noexcept {
    return ok();
  }

  friend bool operator==(result const &lhs, result const &rhs) noexcept;
  friend bool operator!=(result const &lhs, result const &rhs) noexcept;

  friend std::ostream &operator<<(std::ostream &o, result const &r) noexcept;
};

LIBIPC_NAMESPACE_END_

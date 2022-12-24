
#include "libimp/result.h"
#include "libimp/horrible_cast.h"

LIBIMP_NAMESPACE_BEG_

result_code::result_code() noexcept
  : result_code(false, {}) {}

result_code::result_code(result_code_t value) noexcept
  : result_code(true, value) {}

result_code::result_code(result_type const &value) noexcept
  : result_code(std::get<bool>(value), 
                std::get<result_code_t>(value)) {}

result_code::result_code(bool ok, std::uint64_t code) noexcept
  : status_(code, ok) {}

result_code_t result_code::value() const noexcept {
  return std::get<result_code_t>(status_);
}

bool result_code::ok() const noexcept {
  return std::get<bool>(status_);
}

bool operator==(result_code const &lhs, result_code const &rhs) noexcept {
  return lhs.status_ == rhs.status_;
}

bool operator!=(result_code const &lhs, result_code const &rhs) noexcept {
  return lhs.status_ != rhs.status_;
}

LIBIMP_NAMESPACE_END_

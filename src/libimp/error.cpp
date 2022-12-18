#include <typeinfo>

#include "libimp/error.h"
#include "libimp/fmt.h"

LIBIMP_NAMESPACE_BEG_

bool error_category::operator==(error_category const &rhs) const noexcept {
  return typeid(*this) == typeid(rhs);
}

namespace {

class generic_error_category : public error_category {
public:
  std::string message(result_code r) const {
    return fmt("[", r.value(), (!r ? ", \"success\"]" : ", \"failure\"]"));
  }
};

} // namespace

error_category const &generic_category() noexcept {
  static generic_error_category ec;
  return ec;
}

error_code::error_code() noexcept
  : r_code_{}, ec_{&generic_category()} {}

error_code::error_code(result_code r, error_category const &ec) noexcept
  : r_code_{r}, ec_{&ec} {}

result_code error_code::code() const noexcept {
  return r_code_;
}

result_type error_code::value() const noexcept {
  return r_code_.value();
}

error_category const &error_code::category() const noexcept {
  return *ec_;
}

std::string error_code::message() const {
  return ec_->message(r_code_);
}

error_code::operator bool() const noexcept {
  return !!r_code_;
}

bool operator==(error_code const &lhs, error_code const &rhs) noexcept {
  return (lhs.r_code_ == rhs.r_code_) && (*lhs.ec_ == *rhs.ec_);
}

bool operator!=(error_code const &lhs, error_code const &rhs) noexcept {
  return !(lhs == rhs);
}

LIBIMP_NAMESPACE_END_

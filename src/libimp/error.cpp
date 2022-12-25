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
  std::string name() const {
    return "generic";
  }
  std::string message(error_code_t const &r) const {
    if (r == error_code_t(-1)) {
      return "0, \"failure\"";
    }
    if (r == error_code_t(0)) {
      return "0, \"success\"";
    }
    return fmt(r, ", \"failure\"");
  }
};

} // namespace

error_category const &generic_category() noexcept {
  static generic_error_category ec;
  return ec;
}

error_code::error_code() noexcept
  : error_code{0, generic_category()} {}

error_code::error_code(error_code_t const &r) noexcept
  : error_code{r, generic_category()} {}

error_code::error_code(error_code_t const &r, error_category const &ec) noexcept
  : e_code_{r}, ec_{&ec} {}

error_code_t error_code::code() const noexcept {
  return e_code_;
}

error_category const &error_code::category() const noexcept {
  return *ec_;
}

std::string error_code::message() const {
  return fmt("[", ec_->name(), ": ", ec_->message(e_code_), "]");
}

error_code::operator bool() const noexcept {
  return e_code_ != 0;
}

bool operator==(error_code const &lhs, error_code const &rhs) noexcept {
  return (lhs.e_code_ == rhs.e_code_) && (*lhs.ec_ == *rhs.ec_);
}

bool operator!=(error_code const &lhs, error_code const &rhs) noexcept {
  return !(lhs == rhs);
}

LIBIMP_NAMESPACE_END_

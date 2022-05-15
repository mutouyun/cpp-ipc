
#include "libimp/result.h"
#include "libimp/horrible_cast.h"

LIBIMP_NAMESPACE_BEG_
namespace {

struct result_code_info {
  std::uint64_t ok  : 1;
  std::uint64_t code: 63;
};

std::uint64_t make_status(bool ok, std::uint64_t code) noexcept {
  return horrible_cast<std::uint64_t>(result_code_info{(ok ? 1ull : 0ull), code});
}

result_code_info info_cast(std::uint64_t status) noexcept {
  return horrible_cast<result_code_info>(status);
}

} // namespace

result_code::result_code() noexcept
  : result_code(false) {}

result_code::result_code(bool ok, std::uint64_t code) noexcept
  : status_(make_status(ok, code)) {}

std::uint64_t result_code::value() const noexcept {
  return info_cast(status_).code;
}

bool result_code::ok() const noexcept {
  return info_cast(status_).ok != 0;
}

bool operator==(result_code const &lhs, result_code const &rhs) noexcept {
  return lhs.status_ == rhs.status_;
}

bool operator!=(result_code const &lhs, result_code const &rhs) noexcept {
  return lhs.status_ != rhs.status_;
}

LIBIMP_NAMESPACE_END_

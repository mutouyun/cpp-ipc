
#include "libipc/result.h"
#include "libipc/utility/horrible_cast.h"

LIBIPC_NAMESPACE_BEG_
namespace {

struct result_code_info {
  std::uint64_t ok  : 1;
  std::uint64_t code: 63;
};

std::uint64_t make_status(bool ok, std::uint64_t code) noexcept {
  return horrible_cast<std::uint64_t>(result_code_info{ok ? 1ull : 0ull, code});
}

result_code_info info_cast(std::uint64_t status) noexcept {
  return horrible_cast<result_code_info>(status);
}

} // namespace

result::result() noexcept
  : result(false, 0) {}

result::result(bool ok, std::uint64_t code) noexcept
  : status_(make_status(ok, code)) {}

std::uint64_t result::code() const noexcept {
  return info_cast(status_).code;
}

bool result::ok() const noexcept {
  return info_cast(status_).ok != 0;
}

bool operator==(result const &lhs, result const &rhs) noexcept {
  return lhs.status_ == rhs.status_;
}

bool operator!=(result const &lhs, result const &rhs) noexcept {
  return lhs.status_ != rhs.status_;
}

std::ostream &operator<<(std::ostream &o, result const &r) noexcept {
  o << "[" << (r.ok() ? "succ" : "fail") << ", code = " << r.code() << "]";
  return o;
}

LIBIPC_NAMESPACE_END_

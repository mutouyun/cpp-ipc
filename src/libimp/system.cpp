
#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
# include "libimp/platform/win/system.h"
#else
# include "libimp/platform/posix/system.h"
#endif

LIBIMP_NAMESPACE_BEG_
namespace sys {

std::string error_msg(result_code code) noexcept {
  LIBIMP_TRY {
    return fmt("[", code.value(), ", \"", error_str(code), "\"]");
  } LIBIMP_CATCH(...) {
    return error_str(code);
  }
}

/// @brief The system error object.

error::error() noexcept
  : error(error_code()) {}

error::error(result_code rc) noexcept
  : r_code_(rc) {}

result_code error::code() const noexcept {
  return r_code_;
}

std::uint64_t error::value() const noexcept {
  return r_code_.value();
}

error::operator bool() const noexcept {
  return !!r_code_;
}

std::string error::str() const noexcept {
  return error_str(r_code_);
}

bool operator==(error const &lhs, error const &rhs) noexcept {
  return lhs.code() == rhs.code();
}

bool operator!=(error const &lhs, error const &rhs) noexcept {
  return lhs.code() != rhs.code();
}

} // namespace sys
LIBIMP_NAMESPACE_END_

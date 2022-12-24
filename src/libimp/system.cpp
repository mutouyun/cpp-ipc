
#include "libimp/system.h"
#include "libimp/fmt.h"
#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
# include "libimp/platform/win/system.h"
#else
# include "libimp/platform/posix/system.h"
#endif

namespace {

class system_error_category : public ::LIBIMP::error_category {
public:
  std::string name() const {
    return "system";
  }
  std::string message(::LIBIMP::error_code_t const &r) const {
    return ::LIBIMP::fmt(r, ::LIBIMP::sys::error_str(r));
  }
};

} // namespace

LIBIMP_NAMESPACE_BEG_
namespace sys {

error_category const &category() noexcept {
  static system_error_category ec;
  return ec;
}

error_code error() noexcept {
  return error_code{error_no(), category()};
}

} // namespace sys
LIBIMP_NAMESPACE_END_

#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
#include "libimp/platform/win/system.h"
#else
#include "libimp/platform/posix/system.h"
#endif

#include "fmt/format.h"

LIBIMP_NAMESPACE_BEG_
namespace sys {

std::string error_msg(result_code code) noexcept {
  try {
    return ::fmt::format("[{}, \"{}\"]", code.value(), error_str(code));
  } catch (...) {
    return error_str(code);
  }
}

} // namespace sys
LIBIMP_NAMESPACE_END_

#include "libimp/log.h"

#include <cstdio>

LIBIMP_NAMESPACE_BEG_
namespace log {

std_t std_out;

void std_t::output(log::level l, std::string &&s) noexcept {
  switch (l) {
  case level::trace:
  case level::debug:
  case level::info:
    std::fprintf(stdout, "%s\n", s.c_str());
    break;
  case level::warning:
  case level::error:
  case level::failed:
    std::fprintf(stderr, "%s\n", s.c_str());
    break;
  default:
    break;
  }
}

} // namespace log
LIBIMP_NAMESPACE_END_

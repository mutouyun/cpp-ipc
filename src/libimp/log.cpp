
#include <utility>
#include <cstdio>

#include "libimp/log.h"

LIBIMP_NAMESPACE_BEG_
namespace log {

printer::operator bool() const noexcept {
  return (objp_ != nullptr) && (vtable_ != nullptr);
}

void printer::output(log::level l, std::string &&s) {
  if (!*this) return;
  vtable_->output(objp_, l, std::move(s));
}

std_t std_out;

void std_t::output(log::level l, std::string &&s) {
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

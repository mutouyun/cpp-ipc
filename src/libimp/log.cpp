
#include <utility>
#include <cstdio>

#include "libimp/log.h"

LIBIMP_NAMESPACE_BEG_

log_printer::operator bool() const noexcept {
  return (objp_ != nullptr) && (vtable_ != nullptr);
}

void log_printer::info(std::string && s) {
  if (!*this) return;
  vtable_->info(objp_, std::move(s));
}

void log_printer::error(std::string && s) {
  if (!*this) return;
  vtable_->error(objp_, std::move(s));
}

log_std_t log_std;

void log_std_t::info(std::string && s) const {
  std::fprintf(stdin, "%s", s.c_str());
}

void log_std_t::error(std::string && s) const {
  std::fprintf(stderr, "%s", s.c_str());
}

LIBIMP_NAMESPACE_END_

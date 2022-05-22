
#include <utility>
#include <iostream>

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
  std::cout << std::move(s);
}

void log_std_t::error(std::string && s) const {
  std::cerr << std::move(s);
}

LIBIMP_NAMESPACE_END_

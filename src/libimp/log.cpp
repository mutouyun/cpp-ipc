#include "libimp/log.h"

#include <utility>
#include <cstdio>

LIBIMP_NAMESPACE_BEG_
namespace log {

bool to_string(fmt_context &f_ctx, context &&l_ctx) noexcept {
  constexpr static char types[] = {
    'T', 'D', 'I', 'W', 'E', 'F',
  };
  LIBIMP_TRY {
    auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(l_ctx.tp).time_since_epoch().count() % 1000;
    return fmt_to(f_ctx, "[", types[enum_cast(l_ctx.level)], "][", l_ctx.tp, ".", spec("03")(ms), "][", l_ctx.func, "] ", l_ctx.text);
  } LIBIMP_CATCH(std::exception const &) {
    return false;
  }
}

std::string to_string(context &&ctx) noexcept {
  std::string log_txt;
  fmt_context f_ctx(log_txt);
  if (!to_string(f_ctx, std::move(ctx))) {
    return {};
  }
  f_ctx.finish();
  return log_txt;
}

printer::operator bool() const noexcept {
  return (objp_ != nullptr) && (vtable_ != nullptr);
}

void printer::output(context ctx) noexcept {
  if (!*this) return;
  vtable_->output(objp_, std::move(ctx));
}

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

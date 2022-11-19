
#include <utility>
#include <cstdio>

#include "fmt/chrono.h"

#include "libimp/log.h"

LIBIMP_NAMESPACE_BEG_
namespace log {

std::string to_string(context &&ctx) noexcept {
  constexpr static char types[] = {
    'T', 'D', 'I', 'W', 'E', 'F',
  };
  LIBIMP_TRY {
    auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(ctx.tp).time_since_epoch().count() % 1000;
    return ::fmt::format("[{}][{:%Y-%m-%d %H:%M:%S}.{:03}][{}] {}", types[enum_cast(ctx.level)], ctx.tp, ms, ctx.func, ctx.text);
  } LIBIMP_CATCH(std::exception const &e) {
    /// @remark [TBD] std::string constructor may throw an exception
    return e.what();
  }
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

#include "libimp/fmt.h"

#include <cstdio>     // std::snprintf
#include <iomanip>    // std::put_time
#include <sstream>    // std::ostringstream
#include <array>
#include <cstring>    // std::memcpy
#include <algorithm>  // std::min
#include <initializer_list>

#include "libimp/codecvt.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief Format conversions helpers.
 * @see http://personal.ee.surrey.ac.uk/Personal/R.Bowden/C/printf.html
 *      https://en.cppreference.com/w/cpp/io/c/fprintf
 */
namespace {

struct sfmt_policy {
  constexpr static std::size_t aligned_size = 32U;
};

struct sbuf_policy {
  constexpr static std::size_t aligned_size = 2048U;
};

template <typename Policy = sfmt_policy>
span<char> local_fmt_sbuf() noexcept {
  thread_local std::array<char, Policy::aligned_size> sbuf;
  return sbuf;
}

span<char const> normalize(span<char const> const &a) {
  if (a.empty()) return {};
  return a.first(a.size() - (a.back() == '\0' ? 1 : 0));
}

span<char> smem_cpy(span<char> const &sbuf, span<char const> a) noexcept {
  if (sbuf.empty()) return {};
  a = normalize(a);
  auto sz = (std::min)(sbuf.size() - 1, a.size());
  if (sz != 0) std::memcpy(sbuf.data(), a.data(), sz);
  return sbuf.first(sz);
}

span<char> sbuf_cpy(span<char> sbuf, span<char const> const &a) noexcept {
  sbuf = smem_cpy(sbuf, a);
  *sbuf.end() = '\0';
  return sbuf;
}

span<char> sbuf_cat(span<char> const &sbuf, std::initializer_list<span<char const>> args) noexcept {
  std::size_t remain = sbuf.size();
  for (auto s : args) {
    remain -= smem_cpy(sbuf.last(remain), s).size();
  }
  auto sz = sbuf.size() - remain;
  sbuf[sz] = '\0';
  return sbuf.first(sz);
}

char const *as_cstr(span<char const> const &a) {
  if (a.empty()) return "";
  if (a.back() == '\0') return a.data();
  return sbuf_cpy(local_fmt_sbuf(), a).data();
}

span<char> fmt_of(span<char const> const &fstr, span<char const> const &s) {
  return sbuf_cat(local_fmt_sbuf(), {"%", fstr, s});
}

span<char> fmt_of_unsigned(span<char const> fstr, span<char const> const &l) {
  if (fstr.empty()) {
    return fmt_of(l, "u");
  }
  fstr = normalize(fstr);
  switch (fstr.back()) {
    case 'o':
    case 'x':
    case 'X':
    case 'u': return sbuf_cat(local_fmt_sbuf(), {"%", fstr.first(fstr.size() - 1), l, fstr.last(1)});
    default : return sbuf_cat(local_fmt_sbuf(), {"%", fstr, l, "u"});
  }
}

span<char> fmt_of_signed(span<char const> fstr, span<char const> const &l) {
  if (fstr.empty()) {
    return fmt_of(l, "d");
  }
  fstr = normalize(fstr);
  switch (fstr.back()) {
    case 'o':
    case 'x':
    case 'X':
    case 'u': return fmt_of_unsigned(fstr, l);
    default : return sbuf_cat(local_fmt_sbuf(), {"%", fstr, l, "d"});
  }
}

span<char> fmt_of_float(span<char const> fstr, span<char const> const &l) {
  if (fstr.empty()) {
    return fmt_of(l, "f");
  }
  fstr = normalize(fstr);
  switch (fstr.back()) {
    case 'e':
    case 'E':
    case 'g':
    case 'G': return sbuf_cat(local_fmt_sbuf(), {"%", fstr.first(fstr.size() - 1), l, fstr.last(1)});
    default : return sbuf_cat(local_fmt_sbuf(), {"%", fstr, l, "f"});
  }
}

template <typename A /*a fundamental or pointer type*/>
int sprintf(fmt_context &ctx, span<char const> const &sfmt, A a) {
  for (int sz = -1;;) {
    auto sbuf = ctx.buffer(sz + 1);
    if (sbuf.size() < (sz + 1)) {
      return -1;
    }
    sz = std::snprintf(sbuf.data(), sbuf.size(), sfmt.data(), a);
    if (sz <= 0) {
      return sz;
    }
    if (sz < sbuf.size()) {
      ctx.expend(sz);
      return sz;
    }
  }
}

template <typename F /*a function pointer*/, 
          typename A /*a fundamental or pointer type*/>
bool sprintf(fmt_context &ctx, F fop, span<char const> const &fstr, span<char const> const &s, A a) noexcept {
  LIBIMP_TRY {
    return ::LIBIMP::sprintf(ctx, fop(fstr, s), a) >= 0;
  } LIBIMP_CATCH(...) {
    return false;
  }
}

span<char> fmt_context_sbuf() noexcept {
  return local_fmt_sbuf<sbuf_policy>();
}

} // namespace

/// @brief The context of fmt.

fmt_context::fmt_context(std::string &j) noexcept
  : joined_(j)
  , offset_(0) {}

std::size_t fmt_context::capacity() noexcept {
  return (offset_ < fmt_context_sbuf().size())
                  ? fmt_context_sbuf().size()
                  : joined_.size();
}

void fmt_context::reset() noexcept {
  offset_ = 0;
}

bool fmt_context::finish() noexcept {
  LIBIMP_TRY {
    if (offset_ < fmt_context_sbuf().size()) {
      joined_.assign(fmt_context_sbuf().data(), offset_);
    } else {
      joined_.resize(offset_);
    }
    return true;
  } LIBIMP_CATCH(...) {
    return false;
  }
}

span<char> fmt_context::buffer(std::size_t sz) noexcept {
  auto roundup = [](std::size_t sz) noexcept {
    constexpr std::size_t fmt_context_aligned_size = 512U;
    return (sz & ~(fmt_context_aligned_size - 1)) + fmt_context_aligned_size;
  };
  auto sbuf = fmt_context_sbuf();
  LIBIMP_TRY {
    if (offset_ < sbuf.size()) {
      if ((offset_ + sz) < sbuf.size()) {
        return sbuf.subspan(offset_);
      } else {
        /// @remark switch the cache to std::string
        joined_.assign(sbuf.data(), offset_);
        joined_.resize(roundup(offset_ + sz));
      }
    } else if ((offset_ + sz) >= joined_.size()) {
      joined_.resize(roundup(offset_ + sz));
    }
    return {&joined_[offset_], joined_.size() - offset_};
  } LIBIMP_CATCH(...) {
    return {};
  }
}

void fmt_context::expend(std::size_t sz) noexcept {
  offset_ += sz;
}

bool fmt_context::append(std::string const &str) noexcept {
  auto sbuf = buffer(str.size());
  if (sbuf.size() < str.size()) {
    return false;
  }
  std::memcpy(sbuf.data(), str.data(), str.size());
  offset_ += str.size();
  return true;
}

/// @brief To string conversion.

bool to_string(fmt_context &ctx, char const *a) noexcept {
  return to_string(ctx, a, {});
}

bool to_string(fmt_context &ctx, std::string const &a) noexcept {
  return ctx.append(a);
}

bool to_string(fmt_context &ctx, char const *a, span<char const> fstr) noexcept {
  if (a == nullptr) return false;
  return ::LIBIMP::sprintf(ctx, fmt_of, fstr, "s", a);
}

bool to_string(fmt_context &ctx, char a) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of, {}, "c", a);
}

bool to_string(fmt_context &ctx, wchar_t a) noexcept {
  LIBIMP_TRY {
    std::string des;
    cvt_sstr(std::wstring{a}, des);
    return ctx.append(des);
  } LIBIMP_CATCH(...) {
    return false;
  }
}

bool to_string(fmt_context &ctx, char16_t a) noexcept {
  LIBIMP_TRY {
    std::string des;
    cvt_sstr(std::u16string{a}, des);
    return ctx.append(des);
  } LIBIMP_CATCH(...) {
    return false;
  }
}

bool to_string(fmt_context &ctx, char32_t a) noexcept {
  LIBIMP_TRY {
    std::string des;
    cvt_sstr(std::u32string{a}, des);
    return ctx.append(des);
  } LIBIMP_CATCH(...) {
    return false;
  }
}

bool to_string(fmt_context &ctx, signed short a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_signed, fstr, "h", a);
}

bool to_string(fmt_context &ctx, unsigned short a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_unsigned, fstr, "h", a);
}

bool to_string(fmt_context &ctx, signed int a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_signed, fstr, "", a);
}

bool to_string(fmt_context &ctx, unsigned int a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_unsigned, fstr, "", a);
}

bool to_string(fmt_context &ctx, signed long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_signed, fstr, "l", a);
}

bool to_string(fmt_context &ctx, unsigned long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_unsigned, fstr, "l", a);
}

bool to_string(fmt_context &ctx, signed long long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_signed, fstr, "ll", a);
}

bool to_string(fmt_context &ctx, unsigned long long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_unsigned, fstr, "ll", a);
}

bool to_string(fmt_context &ctx, double a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_float, fstr, "", a);
}

bool to_string(fmt_context &ctx, long double a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(ctx, fmt_of_float, fstr, "L", a);
}

bool to_string(fmt_context &ctx, std::nullptr_t) noexcept {
  return ctx.append("null");
}

template <>
bool to_string<void, void>(fmt_context &ctx, void const volatile *a) noexcept {
  if (a == nullptr) {
    return to_string(ctx, nullptr);
  }
  return ::LIBIMP::sprintf(ctx, fmt_of, "", "p", a);
}

bool to_string(fmt_context &ctx, std::tm const &a, span<char const> fstr) noexcept {
  if (fstr.empty()) {
    fstr = "%Y-%m-%d %H:%M:%S";
  }
  LIBIMP_TRY {
    std::ostringstream ss;
    ss << std::put_time(&a, as_cstr(fstr));
    return ctx.append(ss.str());
  } LIBIMP_CATCH(...) {
    return {};
  }
}

LIBIMP_NAMESPACE_END_

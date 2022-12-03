#include "libimp/fmt.h"

#include <cstdio>   // std::snprintf
#include <iomanip>  // std::put_time
#include <sstream>  // std::ostringstream

#include "libimp/codecvt.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief Format conversions helpers.
 * @see http://personal.ee.surrey.ac.uk/Personal/R.Bowden/C/printf.html
 *      https://en.cppreference.com/w/cpp/io/c/fprintf
 */
namespace {

span<char const> normalize(span<char const> a) {
  if (a.empty()) return {};
  return a.first(a.size() - (a.back() == '\0' ? 1 : 0));
}

std::string as_string(span<char const> a) {
  if (a.empty()) return {};
  a = normalize(a);
  return std::string(a.data(), a.size());
}

std::string fmt_of(span<char const> fstr, span<char const> s) {
  return '%' + as_string(fstr) + as_string(s);
}

std::string fmt_of_unsigned(span<char const> fstr, span<char const> l) {
  if (fstr.empty()) {
    return fmt_of(l, "u");
  }
  fstr = normalize(fstr);
  switch (fstr.back()) {
    case 'o':
    case 'x':
    case 'X':
    case 'u': return fmt_of(fstr.first(fstr.size() - 1), l) + fstr.back();
    default : return fmt_of(fstr, l) + 'u';
  }
}

std::string fmt_of_signed(span<char const> fstr, span<char const> l) {
  if (fstr.empty()) {
    return fmt_of(l, "d");
  }
  fstr = normalize(fstr);
  switch (fstr.back()) {
    case 'o':
    case 'x':
    case 'X':
    case 'u': return fmt_of_unsigned(fstr, l);
    default : return fmt_of(fstr, l) + 'd';
  }
}

std::string fmt_of_float(span<char const> fstr, span<char const> l) {
  if (fstr.empty()) {
    return fmt_of(l, "f");
  }
  fstr = normalize(fstr);
  switch (fstr.back()) {
    case 'e':
    case 'E':
    case 'g':
    case 'G': return fmt_of(fstr.first(fstr.size() - 1), l) + fstr.back();
    default : return fmt_of(fstr, l) + 'f';
  }
}

template <typename A /*a fundamental or pointer type*/>
std::string sprintf(std::string const &sfmt, A a) {
  char sbuf[2048];
  auto sz = std::snprintf(sbuf, sizeof(sbuf), sfmt.c_str(), a);
  if (sz <= 0) return {};
  if (sz < sizeof(sbuf)) {
    return sbuf;
  }
  std::string des;
  des.resize(sz + 1);
  if (std::snprintf(&des[0], des.size(), sfmt.c_str(), a) < 0) {
    return {};
  }
  des.pop_back(); // remove the terminated null character
  return des;
}

template <typename F /*a function pointer*/, 
          typename A /*a fundamental or pointer type*/>
std::string sprintf(F fop, span<char const> fstr, span<char const> s, A a) noexcept {
  LIBIMP_TRY {
    return ::LIBIMP::sprintf(fop(fstr, s), a);
  } LIBIMP_CATCH(...) {
    return {};
  }
}

} // namespace

std::string to_string(char const *a, span<char const> fstr) noexcept {
  if (a == nullptr) return {};
  return ::LIBIMP::sprintf(fmt_of, fstr, "s", a);
}

std::string to_string(wchar_t a) noexcept {
  LIBIMP_TRY {
    std::string des;
    cvt_sstr(std::wstring{a}, des);
    return des;
  } LIBIMP_CATCH(...) {
    return {};
  }
}

std::string to_string(char16_t a) noexcept {
  LIBIMP_TRY {
    std::string des;
    cvt_sstr(std::u16string{a}, des);
    return des;
  } LIBIMP_CATCH(...) {
    return {};
  }
}

std::string to_string(char32_t a) noexcept {
  LIBIMP_TRY {
    std::string des;
    cvt_sstr(std::u32string{a}, des);
    return des;
  } LIBIMP_CATCH(...) {
    return {};
  }
}

std::string to_string(signed short a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_signed, fstr, "h", a);
}

std::string to_string(unsigned short a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_unsigned, fstr, "h", a);
}

std::string to_string(signed int a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_signed, fstr, "", a);
}

std::string to_string(unsigned int a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_unsigned, fstr, "", a);
}

std::string to_string(signed long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_signed, fstr, "l", a);
}

std::string to_string(unsigned long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_unsigned, fstr, "l", a);
}

std::string to_string(signed long long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_signed, fstr, "ll", a);
}

std::string to_string(unsigned long long a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_unsigned, fstr, "ll", a);
}

std::string to_string(double a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_float, fstr, "", a);
}

std::string to_string(long double a, span<char const> fstr) noexcept {
  return ::LIBIMP::sprintf(fmt_of_float, fstr, "L", a);
}

template <>
std::string to_string<void, void>(void const volatile *a) noexcept {
  if (a == nullptr) {
    return to_string(nullptr);
  }
  return ::LIBIMP::sprintf(fmt_of, "", "p", a);
}

std::string to_string(std::tm const &a, span<char const> fstr) noexcept {
  if (fstr.empty()) {
    fstr = "%Y-%m-%d %H:%M:%S";
  }
  LIBIMP_TRY {
    std::ostringstream ss;
    ss << std::put_time(&a, as_string(fstr).c_str());
    return ss.str();
  } LIBIMP_CATCH(...) {
    return {};
  }
}

LIBIMP_NAMESPACE_END_
